#include "channel.hpp"
#include "../server.hpp"

#include <converter.hpp>

Irc::Channel::Channel(Server& server, const std::string& name,
		const UnicodeString& key) :
	server_(server), name_(name), key_(key), isJoined_(false)
{
	Join();
}

Irc::Channel::~Channel()
{
	Leave();
}

void Irc::Channel::Join()
{
	std::string command = "JOIN " + name_;
	if (key_.length() > 0)
	{
		command += " " + AsUtf8(key_);
	}
	server_.Send(command);
}

void Irc::Channel::Leave()
{
	if (isJoined_)
	{
		server_.Send("PART " + name_);
	}
}
