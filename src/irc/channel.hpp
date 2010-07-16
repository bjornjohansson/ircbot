#pragma once

#include <unicode/unistr.h>

class Server;

namespace Irc
{

class Channel
{
public:
    Channel(Server& server, const std::string& name, const UnicodeString& key);
    virtual ~Channel();

private:
    void Join();
    void Leave();

    Server& server_;

    std::string name_;
    UnicodeString key_;
    bool isJoined_;
};

} // namespace Irc
