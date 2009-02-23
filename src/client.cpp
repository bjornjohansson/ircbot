#include "client.hpp"
#include "exception.hpp"
#include "searchidle.h"
#include "glue/gluemanager.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>

#include <sys/utsname.h>

const std::string VersionName = "Uno";
const std::string VersionVersion = "$Revision$";
std::string VersionEnvironment = "Unknown";

Client::Client(const std::string& config)
    : config_(config)
{
    struct utsname name;
    if ( uname(&name) == 0 )
    {
	std::stringstream ss;
	ss<<name.sysname<<" "<<name.release;
	VersionEnvironment = ss.str();
    }

    InitLua();
    try
    {
	for(Config::ServerIterator i = config_.GetServersBegin();
	    i != config_.GetServersEnd();
	    ++i)
	{
	    serverSettings_[i->GetId()] = *i;
	    Server& server = Connect(i->GetId(), i->GetHost(), i->GetPort(),
				     i->GetNick());
/*
	    server.ChangeNick(i->GetNick());
	    server.SendUserString(i->GetNick(), i->GetNick());
*/
	    for(Config::Server::ChannelIterator j = i->GetChannelsBegin();
		j != i->GetChannelsEnd();
		++j)
	    {
		server.JoinChannel(j->first, j->second);
	    }
	}
    }
    catch ( Exception& e )
    {
	std::clog<<e.GetMessage()<<std::endl;
    }
}

void Client::Run()
{
    while ( servers_.size() > 0  )
    {
	sleep(30000);
    }
}

void Client::Receive(Server& server, const Irc::Message& message)
{
    if ( message.GetCommand() == "PING" )
    {
	server.Send("PONG "+server.GetHostName());
    }
    else if ( message.GetCommand() == "PRIVMSG" && message.size() >= 2)
    {
	OnPrivMsg(server, message);
    }
}

void Client::JoinChannel(const std::string& serverId,
			 const std::string& channel,
			 const std::string& key)
{
    GetServerFromId(serverId).JoinChannel(channel, key);
}
 
void Client::ChangeNick(const std::string& serverId,
			const std::string& nick)
{
    GetServerFromId(serverId).ChangeNick(nick);
}

void Client::SendMessage(const std::string& serverId,
			 const std::string& target,
			 const std::string& message)
{
    GetServerFromId(serverId).SendMessage(target, message);
}

Client::MessageEventReceiverHandle Client::RegisterForMessages(
    MessageEventReceiver r)
{
    MessageEventReceiverHandle handle(new MessageEventReceiver(r));
    messageEventReceivers_.push_back(MessageEventReceiverPtr(handle));
    return handle;
}

const Config& Client::GetConfig() const
{
    return config_;
}

std::string Client::GetLogName(const std::string& serverId,
			       const std::string& target) const
{
    return GetServerFromId(serverId).GetLogName(target);
}

std::string Client::GetLastLine(const std::string& serverId,
				const std::string& channel,
				const std::string& nick,
				long& timestamp) const
{
    std::string logName = GetServerFromId(serverId).GetLogName(channel);

    std::vector<char> buffer(1024,0);

    int res = searchidle(logName.c_str(), nick.c_str(), &timestamp,
			     &buffer[0], buffer.size());

    std::string result;
    if ( res == 0 )
    {
	result = &buffer[0];
    }
    else
    {
	timestamp = -1;
    }
    
    return result;
}

const std::string& Client::GetNick(const std::string& serverId)
{
    return GetServerFromId(serverId).GetNick();
}

const Server::NickContainer&
Client::GetChannelNicks(const std::string& serverId,
			const std::string& channel)
{
    return GetServerFromId(serverId).GetChannelNicks(channel);
}

Server& Client::GetServerFromId(const std::string& id)
{
    ServerHandleMap::iterator server = servers_.find(id);
    if ( server != servers_.end() )
    {
	return server->second.first;
    }
    else
    {
	throw Exception(__FILE__, __LINE__, "No server named '"+id+"'");
    }
}

const Server& Client::GetServerFromId(const std::string& id) const
{
    ServerHandleMap::const_iterator server = servers_.find(id);
    if ( server != servers_.end() )
    {
	return server->second.first;
    }
    else
    {
	throw Exception(__FILE__, __LINE__, "No server named '"+id+"'");
    }
}

Server& Client::Connect(const std::string& id,
			const std::string& host,
			unsigned int port,
			const std::string& nick)
{
    std::string logDirectory = config_.GetLogsDirectory();
    if ( logDirectory.rfind("/") != logDirectory.size()-1 )
    {
	logDirectory += "/";
    }
    logDirectory += id + "/";

    Server server(id, host, port, nick, logDirectory);
    Server::ReceiverHandle handle = server.RegisterReceiver(
	boost::bind(&Client::Receive, this, _1, _2));

    servers_.insert(ServerHandleMap::value_type(id, ServerAndHandle(server,
								    handle)));

    ServerHandleMap::iterator s = servers_.find(id);

    if ( s == servers_.end() )
    {
	throw Exception(__FILE__, __LINE__, "No matching server");
    }

    return s->second.first;
}

void Client::OnPrivMsg(Server& server, const Irc::Message& message)
{
    const std::string& fromNick = message.GetPrefix().GetNick();
    const std::string& to = message[0];
    const std::string& text = message[1];

    if ( text == (server.GetNick()+": reload") ||
	 (to == server.GetNick() && text == "reload") )
    {
	InitLua();
    }
    else if ( text.find("\1VERSION") == 0 )
    {
	server.SendMessage(fromNick, "\1VERSION "+VersionName+" "+
			   VersionVersion+" running on "+
			   VersionEnvironment);
    }
    else
    {
	std::string cleanedMessage = text;
	const std::string ctcpAction = "\1ACTION";
	if ( cleanedMessage.find(ctcpAction) == 0 )
	{
	    cleanedMessage.replace(0, ctcpAction.size(), "* "+fromNick);
	    std::string::size_type pos;
	    if ( (pos = cleanedMessage.rfind("\1")) != std::string::npos )
	    {
		cleanedMessage.erase(pos);
	    }
	}

	for(MessageEventReceiverContainer::iterator i =
		messageEventReceivers_.begin();
	    i != messageEventReceivers_.end();
	    ++i)
	{
	    if ( MessageEventReceiverHandle f = i->lock() )
	    {
		(*f)(server.GetId(), message.GetPrefix(), to, cleanedMessage);
	    }
	}
    }
}

void Client::Log(Server& server,
		 const std::string& target,
		 const std::string& text)
{
    std::string logFile = config_.GetLogsDirectory();
    if ( logFile.rfind("/") != logFile.size()-1 )
    {
	logFile += "/";
    }
    logFile += server.GetHostName() + "/";
//    EnsureDirectoryExists(logFile);
    logFile += target;

    time_t currentTime = time(0);
    LogStreamMap::iterator stream = logStreams_.find(logFile);
    if ( stream == logStreams_.end() )
    {
#ifdef DEBUG
	std::clog<<"Creating '"<<logFile<<"' log stream"<<std::endl;
#endif
	OfstreamPtr newStream(new std::ofstream(logFile.c_str(),
						std::ios::app));
	logStreams_[logFile] = OfstreamAndTimestamp(newStream, currentTime);
	stream = logStreams_.find(logFile);
    }
    if ( stream != logStreams_.end() )
    {
	(*stream->second.first)<<text<<std::endl;
	stream->second.second = currentTime;
    }
    
}

void Client::ManageLogStreams()
{
    time_t currentTime = time(0);
    for(LogStreamMap::iterator i = logStreams_.begin();
	i != logStreams_.end();	)
    {
	// Only keep logstreams alive for a certain amount of time
	if ( i->second.second + 30*60 < currentTime )
	{
#ifdef DEBUG
	    std::clog<<"Closing '"<<i->first<<"' log stream"<<std::endl;
#endif
	    LogStreamMap::iterator next = i;
	    std::advance(next, 1);
	    logStreams_.erase(i);
	    i = next;
	}
	else
	{
	    ++i;
	}
    }
}

void Client::InitLua()
{
    lua_.reset(new Lua(config_.GetScriptsDirectory()));
    GlueManager::Instance().Reset(lua_, this);
    lua_->LoadScripts();
}
