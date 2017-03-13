//
// Guild.cpp
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

#include "Guild.h"
#include "AegisBot.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include "AegisAdmin.h"
#include "AegisOfficial.h"
#include "AuctionBot.h"
#include "ExampleBot.h"


Guild::Guild(AegisBot & bot)
    : bot(bot)
{

}

Guild::~Guild()
{

}

void Guild::processMessage(json obj)
{
    json author = obj["author"];
    uint64_t userid = std::stoll(author["id"].get<string>());

    //if this is my own message, ignore
    if (userid == AegisBot::userId)
        return;

    string avatar = author["avatar"].is_string()?author["avatar"]:"";
    string username = author["username"];
    uint16_t discriminator = std::stoll(author["discriminator"].get<string>());

    uint64_t channel_id = std::stoll(obj["channel_id"].get<string>());
    uint64_t id = std::stoll(obj["id"].get<string>());
    uint64_t nonce = obj["nonce"].is_null()?0:std::stoll(obj["nonce"].get<string>());

    string content = obj["content"];
    bool tts = obj["tts"];
    bool pinned = obj["pinned"];

    //if message came from a bot, ignore
    if (!AegisBot::globalusers.count(userid) || AegisBot::globalusers[userid]->isbot == true)
        return;



#ifdef SELFBOT
    if (userid != 171000788183678976LL)
        return;
#endif

#ifdef _DEBUG
    if (userid == 171000788183678976LL && content.substr(0, 16) == "!?debugsetprefix")
    {
        string setprefix = content.substr(17);
    }
#endif

    if (userid == owner_id)
    {
        //guild owner is talking, do a check if this is the prefix set up command
        //as that needs to be set up first before the bot will function
        try
        {
            if (content.substr(0, 11) == "!?setprefix")
            {
                string setprefix = content.substr(12);
                if (setprefix.size() == 0)
                    channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
                else
                {
                    prefix = setprefix;
                    channellist[channel_id]->sendMessage(Poco::format("Prefix successfully set to `%s`", setprefix));

                    //TODO: set this in a persistent DB to maintain across restarts
                }
                return;
            }
            else if (content.substr(0, 14) == "!?enablemodule")
            {
                string modulename = content.substr(15);

                if (modulename == "admin" && userid != ROOTADMIN)
                {
                    channellist[channel_id]->sendMessage("Not authorized.");
                    return;
                }

                if (addModule(modulename))
                {
                    channellist[channel_id]->sendMessage(Poco::format("[%s] successfully enabled.", modulename));
                    return;
                }
                else
                {
                    channellist[channel_id]->sendMessage(Poco::format("Error adding module [%s] already added or does not exist", modulename));
                    return;
                }
            }
            else if (content.substr(0, 15) == "!?disablemodule")
            {
                string modulename = content.substr(16);
                if (removeModule(modulename))
                {
                    channellist[channel_id]->sendMessage(Poco::format("Module [%s] successfully disabled.", modulename));
                    return;
                }
                else
                {
                    channellist[channel_id]->sendMessage(Poco::format("Error removing module [%s] or does not exist", modulename));
                    return;
                }
            }
        }
        catch (std::exception&e)
        {
            channellist[channel_id]->sendMessage("Invalid arguments. Prefix must have a length greater than 0");
            return;
        }
    }

    if (content.substr(0, prefix.size()) != prefix)
        return;

    boost::char_separator<char> sep{ " " };
    boost::tokenizer<boost::char_separator<char>> tok{ content.substr(prefix.size()), sep };

    string cmd = *tok.begin();

    //check if attachment exists
    if (obj.count("attachments") > 0)
    {
        std::cout << "Attachments check true" << std::endl;
        for (auto & attach : obj["attachments"])
        {
            std::cout << "Attachment found" << obj["url"] << std::endl;
            if (attachmenthandler.second && attachmenthandler.first.enabled)
            {
                shared_ptr<ABMessage> message = make_shared<ABMessage>();
                message->channel = channellist[channel_id];
                message->member = AegisBot::globalusers[userid];
                message->guild = this->shared_from_this();
                message->message_id = id;
                message->obj = obj;
                attachmenthandler.second(message);
                return;
            }
        }
    }

    if (cmdlist.count(cmd))
    {
        //guild owners bypass all restrictions
        if (userid != owner_id)
        {
            //command exists - construct callback object and perform callback
            //but first check if it's enabled
            if (!cmdlist[cmd].first.enabled)
                return;

            //now check access levels
            //a user that does not exist in the access list has a default permission level of 0
            //commands have a default setting of 1 preventing anyone but guild owner from initially
            //running any commands until permissions are set
            if (clientlist[userid].second < cmdlist[cmd].first.level)
            {
                if (!silentperms)
                {
                    channellist[channel_id]->sendMessage("You do not have access to that command.");
                    return;
                }
            }
        }


        shared_ptr<ABMessage> message = make_shared<ABMessage>();
        message->content = content;
        message->channel = channellist[channel_id];
        message->member = AegisBot::globalusers[userid];
        message->guild = this->shared_from_this();
        message->message_id = id;
        message->cmd = cmd;
        cmdlist[cmd].second(message);
        return;
    }
}

void Guild::addCommand(string command, ABMessageCallback callback)
{
    cmdlist[command] = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addCommand(string command, ABCallbackPair callback)
{
    cmdlist[command] = callback;
}

void Guild::removeCommand(string command)
{
    if (cmdlist.count(command) > 0)
    {
        cmdlist.erase(command);
    }
}

//TODO: add support for multiple maybe?
void Guild::addAttachmentHandler(ABMessageCallback callback)
{
    attachmenthandler = ABCallbackPair(ABCallbackOptions(), callback);
}

void Guild::addAttachmentHandler(ABCallbackPair callback)
{
    attachmenthandler = callback;
}

void Guild::removeAttachmentHandler()
{
    attachmenthandler = ABCallbackPair();
}

void Guild::modifyMember(json content, uint64_t guildid, uint64_t memberid, ABMessageCallback callback)
{
    //if (!canSendMessages())
    //    return;
    poco_trace(*(AegisBot::log), "modifyMember() goes through");

    shared_ptr<ABMessage> message = make_shared<ABMessage>();
    message->content = content.dump();
    message->guild = shared_from_this();
    message->endpoint = Poco::format("/guilds/%Lu/members/%Lu", guildid, memberid);
    message->method = "PATCH";
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}

void Guild::createVoice(json content, uint64_t guildid, ABMessageCallback callback)
{
    //if (!canSendMessages())
    //    return;
    poco_trace(*(AegisBot::log), "createVoice() goes through");

    shared_ptr<ABMessage> message = make_shared<ABMessage>();
    message->content = content.dump();
    message->guild = shared_from_this();
    message->endpoint = Poco::format("/guilds/%Lu/channels", guildid);
    message->method = "POST";
    if (callback)
        message->callback = callback;

    ratelimits.putMessage(message);
}

bool Guild::addModule(string modName)
{
    //modules are hardcoded until I design a system to use SOs to load them.

    for (auto & m : modules)
    {
        if (m->name == modName)
        {
            return false;
        }
    }

    if (modName == "default")
    {
        shared_ptr<AegisOfficial> mod = make_shared<AegisOfficial>(bot, shared_from_this());
        modules.push_back(mod);
        mod->initialize();
        return true;
    }
    else if (modName == "auction")
    {
        shared_ptr<AuctionBot> mod = make_shared<AuctionBot>(bot, shared_from_this());
        modules.push_back(mod);
        mod->initialize();
        return true;
    }
    else if (modName == "example")
    {
        shared_ptr<ExampleBot> mod = make_shared<ExampleBot>(bot, shared_from_this());
        modules.push_back(mod);
        mod->initialize();
        return true;
    }
    else if (modName == "admin")
    {
        shared_ptr<AegisAdmin> mod = make_shared<AegisAdmin>(bot, shared_from_this());
        modules.push_back(mod);
        mod->initialize();
        return true;
    }
    return false;
}

bool Guild::removeModule(string modName)
{
    for (auto mod = modules.begin(); mod != modules.end(); ++mod)
    {
        if ((*mod)->name == modName)
        {
            (*mod)->remove();
            modules.erase(mod);
            return true;
        }
    }
    return false;
}

