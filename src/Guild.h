//
// Guild.h
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

#pragma once

#include "AegisBot.h"
#include <map>
#include "Member.h"
#include "Permission.h"
#include "Channel.h"
#include "Role.h"
#include "../lib/json/src/json.hpp"
#include "RateLimits.h"
#include "AegisModule.h"

using std::shared_ptr;
class AegisBot;

using std::string;
using json = nlohmann::json;

class Guild : public Permission, public std::enable_shared_from_this<Guild>
{
public:
    Guild(AegisBot & bot);
    ~Guild();

    AegisBot & bot;

    void processMessage(json obj);
    void addCommand(string command, ABMessageCallback callback);
    void addCommand(string command, ABCallbackPair callback);
    void removeCommand(string command);
    void addAttachmentHandler(ABMessageCallback callback);
    void addAttachmentHandler(ABCallbackPair callback);
    void removeAttachmentHandler();
    void modifyMember(json content, uint64_t guildid, uint64_t memberid, ABMessageCallback callback = ABMessageCallback());
    void createVoice(json content, uint64_t guildid, ABMessageCallback callback = ABMessageCallback());
    int addModule(string modName);
    bool removeModule(string modName);

    //id, <object, accesslevel>
    std::map<uint64_t, std::pair<shared_ptr<Member>, uint16_t>> clientlist;
    std::map<uint64_t, shared_ptr<Channel>> channellist;
    std::map<uint64_t, shared_ptr<Role>> rolelist;

    //TODO: perhaps endpoint objects for storing ratelimits and can share base class of
    //ratelimits and send functionality, but determine path? overcomplicated?

    //current rate limit system is perfect, but does not adequately handle the different endpoints

    //DM            5:5s
    //ChannelMsg    5:5s
    //account-wide  50:10s
    //delete        5:1s
    //bulk          1:1s
    //member        10:10s
    //member nick   1:1s
    //username      2:3600s

    RateLimits ratelimits;

    string prefix = "?";

    uint64_t id = 0;
    string name;
    string icon;
    string splash;
    uint64_t owner_id = 0;
    string region;
    uint64_t afk_channel_id = 0;
    uint32_t afk_timeout = 0;//in seconds
    bool embed_enabled = false;
    uint64_t embed_channel_id = 0;
    uint32_t verification_level = 0;
    uint32_t default_message_notifications = 0;
    uint32_t mfa_level = 0;
    string joined_at;
    bool large = false;
    bool unavailable = true;
    uint32_t member_count = 0;
    //string voice_states;//this is really an array

    bool silenterrors = false;
    bool silentperms = false;
    bool preventbotparse = false;

    //extendable command list. this list allows you to place c++ functions in matching to a command
    //for more than just simple responses
    std::map<std::string, ABCallbackPair> cmdlist = {};
    ABCallbackPair attachmenthandler;

    std::vector<shared_ptr<AegisModule>> modules;
};

