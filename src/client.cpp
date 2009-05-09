#include "client.hpp"
#include "exception.hpp"
#include "searchidle.h"
#include "glue/gluemanager.hpp"
#include "irc/message.hpp"
#include "irc/server.hpp"
#include "lua/lua.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string.hpp>

#include <sys/utsname.h>

const std::string VersionName = "Uno";
const std::string VersionVersion = "$Revision$";
std::string VersionEnvironment = "Unknown";

Client::Client(const std::string& config)
    : config_(config)
    , run_(false)
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

	    for(Config::Server::ChannelIterator j = i->GetChannelsBegin();
		j != i->GetChannelsEnd();
		++j)
	    {
		server.JoinChannel(j->first, j->second);
	    }
	}
	run_ = true;
    }
    catch ( Exception& e )
    {
	std::clog<<e.GetMessage()<<std::endl;
    }

    try
    {
	namedPipe_.reset(new NamedPipe(config_.GetNamedPipeName()));
	pipeReceiver_ = namedPipe_->RegisterReceiver(
	    boost::bind(&Client::ReceivePipeMessage, this, _1));
    }
    catch ( Exception& )
    {
	std::cerr<<"Could not open named pipe"<<std::endl;
    }
}

void Client::Run()
{
    if ( run_ )
    {
	boost::unique_lock<boost::mutex> lock(runMutex_);
	runCondition_.wait(lock);
    }
}

void Client::Receive(Server& server, const Irc::Message& message)
{
    currentServer_ = server.GetId();
    currentReplyTo_.clear();

    const Irc::Command::Command command = message.GetCommand();
    if ( command == Irc::Command::PING )
    {
	server.Send("PONG "+server.GetHostName());
    }
    else if ( command == Irc::Command::PRIVMSG &&
	      message.size() >= 2 &&
	      !OnPrivMsg(server, message) )
    {
	if ( boost::iequals(server.GetNick(), message.GetPrefix().GetNick()) )
	{
	    // We do not process messages from ourself
	    return;
	}
	boost::shared_lock<boost::shared_mutex> lock(receiverMutex_);
	for(EventReceiverContainer::iterator i
		= eventReceivers_[command].begin();
	    i != eventReceivers_[command].end();
	    ++i)
	{
	    if ( EventReceiverHandle f = i->lock() )
	    {
		(*f)(server.GetId(), message);
	    }
	}
    }
}

void Client::JoinChannel(const std::string& channel,
			 const std::string& key,
			 const std::string& serverId)
{
    GetServerFromId(serverId).JoinChannel(channel, key);
}

void Client::Kick(const std::string& user,
		  const std::string& message,
		  const std::string& channel,
		  const std::string& serverId)
{
    const std::string& chan = channel.empty() ? currentReplyTo_ : channel;
    GetServerFromId(serverId).Kick(chan, user, message);
}
 
void Client::ChangeNick(const std::string& nick,
			const std::string& serverId)
{
    GetServerFromId(serverId).ChangeNick(nick);
}

void Client::SendMessage(const std::string& message,
			 const std::string& target,
			 const std::string& serverId)
{
    const std::string& to = target.empty() ? currentReplyTo_ : target;
    GetServerFromId(serverId).SendMessage(to, message);
}

Client::EventReceiverHandle
Client::RegisterForEvent(const Irc::Command::Command& event,
			 EventReceiver receiver)
{
    boost::upgrade_lock<boost::shared_mutex> lock(receiverMutex_);
    EventReceiverHandle handle(new EventReceiver(receiver));
    eventReceivers_[event].push_back(EventReceiverPtr(handle));
    return handle;
}

const Config& Client::GetConfig() const
{
    return config_;
}

std::string Client::GetLogName(const std::string& target,
			       const std::string& serverId) const
{
    const std::string& to = target.empty() ? currentReplyTo_ : target;
    return GetServerFromId(serverId).GetLogName(to);
}

std::string Client::GetLastLine(const std::string& nick,
				long& timestamp,
				const std::string& channel,
				const std::string& serverId) const
{
    const std::string& to = channel.empty() ? currentReplyTo_ : channel;
    std::string logName = GetServerFromId(serverId).GetLogName(to);

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

const std::set<std::string>&
Client::GetChannelNicks(const std::string& channel,
			const std::string& serverId)
{
    const std::string& to = channel.empty() ? currentReplyTo_ : channel;
    return GetServerFromId(serverId).GetChannelNicks(to);
}

Server& Client::GetServerFromId(const std::string& id)
{
    const std::string& serverId = id.empty() ? currentServer_ : id;

    boost::shared_lock<boost::shared_mutex> lock(serverMutex_);
    ServerHandleMap::iterator server = servers_.find(serverId);
    if ( server != servers_.end() )
    {
	return *server->second.first;
    }
    else
    {
	throw Exception(__FILE__, __LINE__, "No server named '"+id+"'");
    }
}

const Server& Client::GetServerFromId(const std::string& id) const
{
    const std::string& serverId = id.empty() ? currentServer_ : id;

    boost::shared_lock<boost::shared_mutex> lock(serverMutex_);
    ServerHandleMap::const_iterator server = servers_.find(serverId);
    if ( server != servers_.end() )
    {
	return *server->second.first;
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

    {
	boost::upgrade_lock<boost::shared_mutex> lock(serverMutex_);

	ServerPtr server(new Server(id, host, port, nick, logDirectory));
	Server::ReceiverHandle handle = server->RegisterReceiver(
	    boost::bind(&Client::Receive, this, _1, _2));
	
	servers_.insert(ServerHandleMap::value_type(id,
						    ServerAndHandle(server,
								    handle)));
    }

    boost::shared_lock<boost::shared_mutex> lock(serverMutex_);
    ServerHandleMap::iterator s = servers_.find(id);

    if ( s == servers_.end() )
    {
	throw Exception(__FILE__, __LINE__, "No matching server");
    }

    return *s->second.first;
}

bool Client::OnPrivMsg(Server& server, const Irc::Message& message)
{
    const std::string& fromNick = message.GetPrefix().GetNick();
    const std::string& to = message[0];
    const std::string& text = message[1];

    if ( text == (server.GetNick()+": reload") ||
	 (to == server.GetNick() && text == "reload") )
    {
	InitLua();
	return true;
    }
    else if ( text == (server.GetNick()+": quit") ||
	      (to == server.GetNick() && text == "quit" ))
    {
        runCondition_.notify_all();
	return true;
    }
    else if ( text.find("\1VERSION") == 0 )
    {
	server.SendMessage(fromNick, "\1VERSION "+VersionName+" "+
			   VersionVersion+" running on "+
			   VersionEnvironment);
	return true;
    }
    return false;
}

void Client::ReceivePipeMessage(const std::string& line)
{
    std::stringstream ss(line);
    std::string command, server, channel, message;
    ss>>command>>server>>channel;
    if ( ss.peek() == ' ' ) { ss.get(); }
    std::getline(ss, message);

    try
    {
	if ( boost::iequals(command, "say") )
	{
	    SendMessage(message, channel, server);
	}
    }
    catch ( Exception& e )
    {
	std::cout<<"Named pipe failed to send message: "<<e.GetMessage()
		 <<std::endl;
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
