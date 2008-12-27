#pragma once

#include <string>

class Server;

namespace Irc
{

class Channel
{
public:
    Channel(Server& server, const std::string& name, const std::string& key);
    virtual ~Channel();

private:
    void Join();
    void Leave();

    Server& server_;

    std::string name_;
    std::string key_;
    bool isJoined_;
};

} // namespace Irc
