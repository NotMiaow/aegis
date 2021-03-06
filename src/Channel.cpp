//
// Channel.cpp
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

#include "Channel.h"
#include "Guild.h"
#include "AegisBot.h"
#include "Member.h"
#include "../lib/fmt/fmt/ostream.h"

ABMessage::ABMessage(Channel * channel)
    : _bot(channel->guild()._bot)
    , _channel(channel)
    , _member(channel->guild().shard().self)
    , _guild(nullptr)
{
    if (channel != nullptr)
    {
        if (channel->ready())
        {
            if (channel->guild().ready())
            {

            }
        }
    }
    //should theoretically never happen
    if (channel->guild()._bot == nullptr)
        throw std::runtime_error(fmt::format("1 ABMessage _bot nullptr channelid {}", channel->id));
}

ABMessage::ABMessage(Channel * channel, Member * member)
    : _bot(channel->guild()._bot)
    , _channel(channel)
    , _member(member)
    , _guild(nullptr)
{
    //should theoretically never happen
    if (channel->guild()._bot == nullptr)
        throw std::runtime_error(fmt::format("2 ABMessage _bot nullptr channelid {}", channel->id));
}

ABMessage::ABMessage(Guild * guild)
    : _bot(guild->_bot)
    , _channel(nullptr)
    , _member(nullptr)
    , _guild(guild)
{
    //should theoretically never happen
    if (guild->_bot == nullptr)
        throw std::runtime_error(fmt::format("3 ABMessage _bot nullptr guildid {}", guild->id));
}

void Channel::getMessages(uint64_t messageid, ABMessageCallback callback)
{
    if (!getPermission().canReadHistory())
    {
        if (guild().silentperms)
            return;
        throw no_permission("READ_HISTORY");
    }

    ABMessage message(this);
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "GET";
    message.query = fmt::format("?before={0}&limit=100", messageid);
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::sendMessage(std::string content, ABMessageCallback callback)
{
    //ignore permissions in hopes the bot has it as it uses sendMessage() to report other
    //permission errors and failures if enabled
//     if (!canSendMessages())
//         return;

    json obj;
    if (guild().preventbotparse)
        obj["content"] = u8"\u200B" + content;
    else
        obj["content"] = content;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::sendMessageEmbed(json content, json embed, ABMessageCallback callback /*= ABMessageCallback()*/)
{
    if (!getPermission().canEmbed())
         return;

    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::bulkDelete(std::vector<std::string> messages, ABMessageCallback callback)
{
    if (!getPermission().canManageMessages())
    {
        if (guild().silentperms)
            return;
        throw no_permission("MANAGE_MESSAGES");
    }

    json arr(messages);
    json obj;
    obj["messages"] = arr;

    ABMessage message(this);
    message.content = obj.dump();
    message.endpoint = fmt::format("/channels/{0}/messages/bulk-delete", id);
    message.method = "POST";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

void Channel::deleteChannel(ABMessageCallback callback)
{
    if (!getPermission().canManageGuild())
    {
        if (guild().silentperms)
            return;
        throw no_permission("MANAGE_SERVER");
    }

    ABMessage message(this);
    message.endpoint = fmt::format("/channels/{0}", id);
    message.method = "DELETE";
    if (callback)
        message.callback = callback;

    ratelimits.putMessage(std::move(message));
}

Permission & Channel::getPermission()
{
    return permission_cache[guild().shard().userId];
}

void Channel::UpdatePermissions()
{
    uint64_t botid = guild().shard().userId;

    permission_cache.clear();

    for (auto & m : guild().memberlist)
    {
        uint64_t allow = 0, deny = 0;

        if (m.second.first == nullptr)
        {
            //user is not included in the GUILD_CREATE so we don't know what roles it has
            //TODO: do something about this
            continue;
        }

        for (auto & p : m.second.first->roles)
        {
            allow |= guild().rolelist[p].permission.getAllowPerms();
            deny |= guild().rolelist[p].permission.getDenyPerms();//roles don't have deny perms do they? they either have it allowed, or not
            if (overrides.count(p))
            {
                allow |= overrides[p].allow;
                deny |= overrides[p].deny;
            }
        }
        if (overrides.count(m.second.first->id))
        {
            allow |= overrides[m.second.first->id].allow;
            deny |= overrides[m.second.first->id].deny;
        }

        allow |= overrides[guild().id].allow;
        deny |= overrides[guild().id].deny;

        permission_cache[m.second.first->id] = Permission(allow, deny);
    }

//     for (auto & m : guild().memberlist)
//     {
//         guild_permission_cache[m.second.first->id] = Permission(allow, deny);
// 
//     }

}

/*
if (!canEmbed())
{
    if (guild().silentperms)
        return;
    else
        throw no_permission("EMBED_LINKS");
}*/
