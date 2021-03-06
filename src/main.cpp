//
// main.cpp
// aegisbot
//
// Copyright (c) 2017 Zero (zero at xandium dot net)
//
// This file is part of aegisbot.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "AegisBot.h"
#if defined USE_REDIS
#define CACHE_PUT(x, y)
#include "ABRedisCache.h"
#elif defined USE_MEMORY
#define CACHE_PUT(x, y) cache.put(x, y)
#include "ABMemoryCache.h"
#endif
#include <boost/lexical_cast.hpp>
#include <json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/tuple/tuple.hpp>

using namespace std::chrono;

int main(int argc, char * argv[])
{
    srand(static_cast<int32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()));
    bool overwritecache = false;

#ifdef WIN32
    Poco::Crypto::OpenSSLInitializer::initialize();
#endif

    if (argc > 1)
        if (!strcmp(argv[1], "-overwritecache"))
            overwritecache = true;


    //////////////////////////////////////////////////////////////////////////
    //EDITABLE AREA
    //default commands to add to a guild
    std::map<std::string, ABMessageCallback> & cmdlist = AegisBot::cmdlist;
    std::map<std::string, ABMessageCallback> & admincmdlist = AegisBot::admincmdlist;

    try
    {
        boost::asio::io_service::work work(AegisBot::io_service);

        cmdlist["perms"] = [](ABMessage & message)
        {
            message.channel().sendMessage(fmt::format("My perms Allow {0:#x} Deny {0:#x} : {0}", message.channel().getPermission().getAllowPerms(), message.channel().getPermission().getDenyPerms()));
        };

        cmdlist["setgame"] = [](ABMessage & message)
        {
            std::string gamestr = message.content.substr(message.channel().guild().prefix.size() + 8);
            AegisBot::io_service.post([game = std::move(gamestr), &bot = message.channel().guild().shard()]()
            {
                json obj = { { { "game",{ { "name", game },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } };

                bot.wssend(obj.dump());
            });
        };

        cmdlist["events"] = [](ABMessage & message)
        {
            uint64_t eventsseen = 0;
            std::stringstream ss;

            ss << "```Total: {0}";

            for (auto & evt : AegisBot::eventCount)
            {
                ss << " [" << evt.first << "]:" << evt.second;
                eventsseen += evt.second;
            }

            ss << "```";

            message.channel().sendMessage(fmt::format(ss.str(), eventsseen));
        };

        cmdlist["info"] = [](ABMessage & message)
        {
            uint64_t guild_count = AegisBot::guildlist.size();
            uint64_t member_count = AegisBot::memberlist.size();
            uint64_t member_count_unique = 0;
            uint64_t member_online_count = 0;
            uint64_t member_dnd_count = 0;
            uint64_t channel_count = AegisBot::channellist.size();
            uint64_t channel_text_count = 0;
            uint64_t channel_voice_count = 0;
            uint64_t member_count_active = 0;

            uint64_t eventsseen = 0;

            {
                std::lock_guard<std::recursive_mutex> lock(AegisBot::m);

                for (auto & bot : AegisBot::shards)
                    eventsseen += bot->sequence;

                for (auto & member : AegisBot::memberlist)
                {
                    if (member.second->status == MEMBER_STATUS::ONLINE)
                        member_online_count++;
                    else if (member.second->status == MEMBER_STATUS::DND)
                        member_dnd_count++;
                }

                for (auto & channel : AegisBot::channellist)
                {
                    if (channel.second->type == ChannelType::TEXT)
                        channel_text_count++;
                    else
                        channel_voice_count++;
                }

                for (auto & guild : AegisBot::guildlist)
                    member_count_active += guild.second->memberlist.size();

                member_count = message.bot().memberlist.size();
            }

            std::string members = fmt::format("{0} seen\n{1} total\n{2} unique\n{3} online\n{4} dnd", member_count, member_count_active, member_count_unique, member_online_count, member_dnd_count);
            std::string channels = fmt::format("{0} total\n{1} text\n{2} voice", channel_count, channel_text_count, channel_voice_count);
            std::string guilds = fmt::format("{0}", guild_count);
            std::string events = fmt::format("{0}", eventsseen);
            std::string misc = fmt::format("I am shard {0} of {1} running on `{2}`", message.channel().guild().shard().shardid + 1, message.channel().guild().shard().shardidmax, PLATFORM_NAME);

            fmt::MemoryWriter w;
            w << "[Latest bot source](https://github.com/zeroxs/aegis)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                << double(AegisBot::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                << double(AegisBot::getPeakRSS()) / (1024 * 1024) << "MB";
            std::string stats = w.str();

            message.content = "";
            json t = {
                { "title", "AegisBot" },
                { "description", stats },
                { "color", rand() % 0xFFFFFF },//10599460 },
                { "fields",
                json::array(
            {
                { { "name", "Members" },{ "value", members },{ "inline", true } },
                { { "name", "Channels" },{ "value", channels },{ "inline", true } },
                { { "name", "Uptime" },{ "value", AegisBot::uptime() },{ "inline", true } },
                { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
                { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                { { "name", "misc" },{ "value", misc },{ "inline", false } }
            }
                    )
                },
                { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
            };
            message.channel().sendMessageEmbed(json(), t);
        };

//         myguild.addCommand("reset", [&bot](ABMessage & message)
//         {
//             message.channel().guild().bot.ws.close(message.channel().guild().bot.hdl, 1002, "");
//         });
        
        
        cmdlist["custom_command"] = [](ABMessage & message)
        {
            message.channel().sendMessage("unique command for this guild.", [](ABMessage & message)
            {
                //continuation from whatever that is
            });
        };

        admincmdlist["echo"] = [](ABMessage & message)
        {
            message.channel().sendMessage(message.content.substr(message.cmd.size() + message.channel().guild().prefix.size()));
        };

        admincmdlist["clearchat"] = [](ABMessage & message)
        {
            message.channel().getMessages(message.message_id, [](ABMessage & message)
            {
                try
                {
                    int64_t epoch = ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - (14 * 24 * 60 * 60 * 1000)) - 1420070400000) << 22;
                    std::vector<std::string> delmessages;
                    for (auto & m : message.obj)
                    {
                        if (std::stoull(m["id"].get<std::string>()) > epoch)
                        {
                            delmessages.push_back(m["id"].get<std::string>());
                        }
                    }
                    message.channel().bulkDelete(delmessages);
                }
                catch (std::exception & e)
                {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            });
        };

        admincmdlist["disc"] = [](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            fmt::MemoryWriter w;
            int i = 0;

            for (auto & m : AegisBot::memberlist)
            {
                if (m.second->discriminator == std::stoi(tokens[1]))
                {
                    ++i;
                    w << m.second->getFullName() << "\n";
                }
            }
            if (i > 0)
                message.channel().sendMessage(w.str());
            else
                message.channel().sendMessage("None");
        };

        admincmdlist["serverlist"] = [](ABMessage & message)
        {
            //fmt::MemoryWriter w;
            std::stringstream w;
            for (auto & g : message.channel().guild().shard().guildlist)
            {
                w << "*" << g.second->name << "*  :  " << g.second->id << "\n";
            }


            json t = {
                { "title", "Server List" },
                { "description", w.str() },
                { "color", 10599460 }
            };
            message.channel().sendMessageEmbed(json(), t);
        };

        admincmdlist["leaveserver"] = [](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            uint64_t guildid = std::stoull(tokens[1]);

            if (tokens.size() >= 2)
            {
                try
                {
                    std::string guildname = message.bot().getGuild(guildid).name;
                    message.channel().sendMessage(fmt::format("Leaving **{1}** [{0}]", tokens[1], message.bot().getGuild(guildid).name));
                    message.bot().getGuild(guildid).leave(std::bind([](ABMessage & message, std::string id, std::string guildname, Channel * channel)
                    {
                        channel->sendMessage(fmt::format("Successfully left **{1}** [{0}]", id, guildname));
                    }, std::placeholders::_1, tokens[1], guildname, &message.channel()));
                }
                catch (std::domain_error & e)
                {
                    message.channel().sendMessage(std::string(e.what()));
                }
            }
            else
            {
                message.channel().sendMessage("Invalid guild id.");
            }
        };

        cmdlist["serverinfo"] = [](ABMessage & message)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message.content, boost::is_any_of(" "));

            if (tokens.size() >= 2)
            {
                uint64_t guildid = std::stoull(tokens[1]);
              
                Guild & guild = message.bot().getGuild(guildid);
                std::string title;

                title = fmt::format("Server: **{0}**", guild.name);

                uint64_t botcount = 0;
                for (auto & m : guild.memberlist)
                    if (m.second.first->isbot)
                        botcount++;
                //TODO: may break until full member chunk is obtained on connect

                //TODO:2 this should be getMember
                Member & owner = guild.shard().getMember(guild.owner_id);
                fmt::MemoryWriter w;
                w << "Owner name: " << owner.getFullName() << " [" << owner.getName(guild.id).value_or("") << "]\n"
                    << "Owner id: " << guild.owner_id << "\n"
                    << "Member count: " << guild.memberlist.size() << "\n"
                    << "Channel count: " << guild.channellist.size() << "\n"
                    << "Bot count: " << botcount << "\n"
                    << "";


                json t = {
                    { "title", title },
                    { "description", w.str() },
                    { "color", 10599460 }
                };
                message.channel().sendMessageEmbed(json(), t);
            }
            else
            {
                message.channel().sendMessage("Invalid guild id.");
            }
        };

        cmdlist["osver"] = [](ABMessage & message)
        {
            //TODO: add more detailed information like actual OS flavor (for linux) and version
            message.channel().sendMessage(fmt::format("I am running on `{}`", PLATFORM_NAME));
        };

        admincmdlist["listor"] = [](ABMessage & message)
        {
            auto arr = json::array();
            for (auto & o : message.channel().overrides)
            {
                arr.push_back(json({ {"name", fmt::format("{}", o.second.type == Override::ROLE ? "Role" : "User")}, {"value", fmt::format("id: {}\nAllow: {}\nDeny: {}", o.first, o.second.allow, o.second.deny) }, {"inline", false} }));
            }
            json msg =
            {
                { "title", "AegisBot" },
                { "description", "Perms" },
                { "color", rand() % 0xFFFFFF },//10599460 },
                { "fields", arr }
            };
            message.channel().sendMessageEmbed(json(), msg);
        };

        admincmdlist["listperms"] = [](ABMessage & message)
        {
            auto & permission = message.channel().getPermission();
            json msg =
            {
                { "title", "AegisBot" },
                { "description", "Perms" },
                { "color", rand() % 0xFFFFFF },//10599460 },
                { "fields",
                json::array(
                    {
                        { { "name", "canInvite" },          { "value", fmt::format("{}", permission.canInvite()) },{ "inline", true } },
                        { { "name", "canKick" },            { "value", fmt::format("{}", permission.canKick()) },{ "inline", true } },
                        { { "name", "canBan" },             { "value", fmt::format("{}", permission.canBan()) },{ "inline", true } },
                        { { "name", "isAdmin" },            { "value", fmt::format("{}", permission.isAdmin()) },{ "inline", true } },
                        { { "name", "canManageChannels" },  { "value", fmt::format("{}", permission.canManageChannels()) },{ "inline", true } },
                        { { "name", "canManageGuild" },     { "value", fmt::format("{}", permission.canManageGuild()) },{ "inline", true } },
                        { { "name", "canAddReactions" },    { "value", fmt::format("{}", permission.canAddReactions()) },{ "inline", true } },
                        { { "name", "canViewAuditLogs" },   { "value", fmt::format("{}", permission.canViewAuditLogs()) },{ "inline", true } },
                        { { "name", "canReadMessages" },    { "value", fmt::format("{}", permission.canReadMessages()) },{ "inline", true } },
                        { { "name", "canSendMessages" },    { "value", fmt::format("{}", permission.canSendMessages()) },{ "inline", true } },
                        { { "name", "canTTS" },             { "value", fmt::format("{}", permission.canTTS()) },{ "inline", true } },
                        { { "name", "canManageMessages" },  { "value", fmt::format("{}", permission.canManageMessages()) },{ "inline", true } },
                        { { "name", "canEmbed" },           { "value", fmt::format("{}", permission.canEmbed()) },{ "inline", true } },
                        { { "name", "canAttachFiles" },     { "value", fmt::format("{}", permission.canAttachFiles()) },{ "inline", true } },
                        { { "name", "canReadHistory" },     { "value", fmt::format("{}", permission.canReadHistory()) },{ "inline", true } },
                        { { "name", "canMentionEveryone" }, { "value", fmt::format("{}", permission.canMentionEveryone()) },{ "inline", true } },
                        { { "name", "canExternalEmoiji" },  { "value", fmt::format("{}", permission.canExternalEmoiji()) },{ "inline", true } },
                        { { "name", "canChangeName" },      { "value", fmt::format("{}", permission.canChangeName()) },{ "inline", true } },
                        { { "name", "canManageNames" },     { "value", fmt::format("{}", permission.canManageNames()) },{ "inline", true } },
                        { { "name", "canManageRoles" },     { "value", fmt::format("{}", permission.canManageRoles()) },{ "inline", true } },
                        { { "name", "canManageWebhooks" },  { "value", fmt::format("{}", permission.canManageWebhooks()) },{ "inline", true } },
                        { { "name", "canManageEmojis" },    { "value", fmt::format("{}", permission.canManageEmojis()) },{ "inline", true } },
                        { { "name", "canMentionEveryone" }, { "value", fmt::format("{}", permission.canMentionEveryone()) },{ "inline", true } },
                        { { "name", "canVoiceConnect" },    { "value", fmt::format("{}", permission.canVoiceConnect()) },{ "inline", true } },
                        { { "name", "canVoiceMute" },       { "value", fmt::format("{}", permission.canVoiceMute()) },{ "inline", true } },
                        { { "name", "canVoiceSpeak" },      { "value", fmt::format("{}", permission.canVoiceSpeak()) },{ "inline", true } },
                        { { "name", "canVoiceDeafen" },     { "value", fmt::format("{}", permission.canVoiceDeafen()) },{ "inline", true } },
                        { { "name", "canVoiceMove" },       { "value", fmt::format("{}", permission.canVoiceMove()) },{ "inline", true } },
                        { { "name", "canVoiceActivity" },   { "value", fmt::format("{}", permission.canVoiceActivity()) },{ "inline", true } }
                    }
                    )
                }
            };
            message.channel().sendMessageEmbed(json(), msg);
        };

        cmdlist["shards"] = [](ABMessage & message)
        {
            std::string s = "```\n";
            for (auto & shard : AegisBot::shards)
            {
                s += fmt::format("shard#{} tracking {:4} guilds {:4} channels {:4} members {:4} messages\n", shard->shardid, shard->counters.guilds, shard->counters.channels, shard->counters.members, shard->counters.messages);
            }
            s += "```";
            message.channel().sendMessage(s);
        };

        //DO NOT EDIT BEYOND THIS LINE
        //////////////////////////////////////////////////////////////////////////


        AegisBot::configure(overwritecache);

        AegisBot::threads();
        AegisBot::workthread.join();
    }
    catch (std::exception & e)
    {
        std::cout << e.what() << std::endl;
        std::cin.ignore().get();
        return -1;
    }
    return 0;
}

