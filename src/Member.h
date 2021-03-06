//
// Member.h
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
#include "Permission.h"
#include "RateLimits.h"
#include <string>
#include <vector>
#include <map>
#include <boost/optional.hpp>

class AegisBot;
class Guild;

enum class MEMBER_STATUS
{
    OFFLINE,
    ONLINE,
    STREAM,
    DND
};

struct stGuildInfo
{
    std::vector<uint64_t> roles;
    std::string nickname;
    Guild * guild;
};

class Member 
{
public:
    Member() {};
    Member(uint64_t id, std::string name, uint16_t discriminator, std::string avatar);
    ~Member();

    std::vector<Guild*> getGuilds();
    boost::optional<std::string> getName(uint64_t guildid);
    std::string getFullName();

    std::map<uint64_t, stGuildInfo> guilds;
    std::vector<uint64_t> channels;
    std::vector<uint64_t> roles;
    std::map<uint64_t, Override> overrides;//channel, struct

    std::queue<uint64_t> msghistory;

    //
    static std::mutex m;

    uint64_t id = 0;
    std::string name;
    uint16_t discriminator = 0;
    std::string avatar;
    bool isbot = false;
    bool deaf = false;
    bool mute = false;
    std::string joined_at;
    MEMBER_STATUS status = MEMBER_STATUS::OFFLINE;

    //RateLimits ratelimits;
};

