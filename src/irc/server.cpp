
#include "server.hpp"
#include "message.hpp"
//#include "../connection/connectionmanager.hpp"
#include "../exception.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string/trim.hpp>

void EnsureDirectoryExists(const std::string& directory)
{
    try
    {
	boost::filesystem::create_directories(directory);
    }
    catch ( boost::filesystem::filesystem_error& )
    {
	std::cerr<<"Could not create directories '"<<directory<<"'"<<std::endl;
    }
}

Server::Server(const std::string& id, 
	       const std::string& host, 
	       unsigned int port,
	       const std::string& nick,
	       const std::string& logDirectory)
    : id_(id),
      host_(host),
      logDirectory_(logDirectory),
      nick_(nick),
      appendingChannelNicks_(false)
{
    RegisterSelfAsReceiver();
    connection_.Connect(host, port);
}
/*
Server::Server(const Server& rhs)
{
    *this = rhs;
}
*/
Server::~Server()
{
    // Do not detroy this class while it's performing callbacks
    boost::lock_guard<boost::mutex> lock(callbackMutex_);
}

/*
Server& Server::operator=(const Server& rhs)
{
    receivers_ = rhs.receivers_;
    connection_ = rhs.connection_;
    receiveBuffer_ = rhs.receiveBuffer_;
    id_ = rhs.id_;
    host_ = rhs.host_;
    nick_ = rhs.nick_;
    logDirectory_ = rhs.logDirectory_;
    channels_ = rhs.channels_;
    channelNicks_ = rhs.channelNicks_;
    appendingChannelNicks_ = rhs.appendingChannelNicks_;

    RegisterSelfAsReceiver();
    return *this;
}
*/
void Server::Send(const std::string& data)
{
    std::vector<char> buffer;
    std::copy(data.begin(), data.end(), std::back_inserter(buffer));
    buffer.push_back('\r');
    buffer.push_back('\n');
    try
    {
	connection_.Send(buffer);
    }
    catch ( Exception& e )
    {
	std::cerr<<e.GetMessage()<<std::endl;
    }
#ifdef DEBUG
    std::clog<<host_<<"> "<<data<<std::endl;
#endif
}

void Server::Receive(Connection& connection, const std::vector<char>& data)
{
    boost::lock_guard<boost::mutex> lock(callbackMutex_);
    // Copy everything but '\r' as some irc servers don't comply
    std::remove_copy(data.begin(), data.end(),
		     std::back_inserter(receiveBuffer_), '\r');

    for(std::string::size_type pos = receiveBuffer_.find('\n');
	pos != std::string::npos;
	pos = receiveBuffer_.find('\n'))
    {
	OnText(receiveBuffer_.substr(0, pos));
	receiveBuffer_.erase(0, pos+1);
    }
}

void Server::OnConnect(Connection& connection)
{
    ChangeNick(nick_);
    SendUserString(nick_, nick_);

    boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
    for(ChannelKeyMap::iterator channel = channels_.begin();
	channel != channels_.end();
	++channel)
    {
	JoinChannel(channel->first, channel->second);
    }
}

const std::string& Server::GetId() const
{
    return id_;
}

const std::string& Server::GetHostName() const
{
    return host_;
}

const std::string& Server::GetNick() const
{
    boost::shared_lock<boost::shared_mutex> lock(nickMutex_);
    return nick_;
}


Server::ReceiverHandle Server::RegisterReceiver(Receiver receiver)
{
    boost::upgrade_lock<boost::shared_mutex> lock(receiversMutex_);
    boost::shared_ptr<Receiver> handle(new Receiver(receiver));
    receivers_.push_back(boost::weak_ptr<Receiver>(handle));
    return handle;
}

std::string Server::GetLogName(const std::string& target) const
{
    std::string logFile = logDirectory_;
    if ( logFile.rfind("/") != logFile.size()-1 )
    {
	logFile += "/";
    }
    EnsureDirectoryExists(logFile);
    logFile += target;
    return logFile;
}

void Server::JoinChannel(const std::string& channel, const std::string& key)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelsMutex_);
    channels_[channel] = key;
    if ( connection_.IsConnected() )
    {
	if ( key.size() > 0 )
	{
	    Send("JOIN "+channel+" "+key);
	}
	else
	{
	    Send("JOIN "+channel);
	}
    }
}

void Server::ChangeNick(const std::string& nick)
{
    Send("NICK "+nick);
    boost::upgrade_lock<boost::shared_mutex> lock(nickMutex_);
    nick_ = nick;
}

void Server::SendUserString(const std::string& user, const std::string name)
{
    Send(std::string("USER ")+user+
	 std::string(" localhost ")+GetHostName()+
	 std::string(" :")+name);
}

void Server::SendMessage(const std::string& target, const std::string& message)
{
    std::string scrubbedMessage = message;
    std::string::size_type pos = std::string::npos;
    if ( (pos=scrubbedMessage.find_first_of("\n\r")) != std::string::npos )
    {
	scrubbedMessage = scrubbedMessage.substr(0, pos);
    }

    if ( scrubbedMessage.size() > 0 )
    {
	Send("PRIVMSG "+target+" :"+scrubbedMessage);

	scrubbedMessage = CleanMessageForDisplay(GetNick(), scrubbedMessage);

	std::stringstream ss;
	ss.imbue(std::locale::classic());
	ss<<time(0)<<" "<<target<<": <"<<GetNick()<<"> "<<scrubbedMessage;
	Log(target ,ss.str());
    }
}

void Server::Kick(const std::string& channel,
		  const std::string& user,
		  const std::string& message)
{
    Send(std::string("KICK ")+channel+
	 std::string(" ")+user+
	 std::string(" :")+(message.empty() ? "" : message));
}

const Server::NickContainer&
Server::GetChannelNicks(const std::string& channel) const
{
    boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
    ChannelNicksContainer::const_iterator channelNicks =
	channelNicks_.find(channel);
    if ( channelNicks != channelNicks_.end() )
    {
	return channelNicks->second;
    }
    throw Exception(__FILE__, __LINE__,
		    std::string("Could not get nicks for channel '")+channel+
		    std::string("'"));
}


template<class T>
bool IsIrcNameStatus(const T& v)
{
    return v == '@' || v == '+';
}

void Server::OnText(const std::string& text)
{
#ifdef DEBUG
    std::clog<<host_<<"< "<<text<<std::endl;
#endif
    Irc::Message message(text);

    // Log messages
    if ( message.GetCommand() == "PRIVMSG" && message.size() >= 2)
    {
	const std::string& from = message.GetPrefix().GetNick();
	const std::string& to = message[0];
	const std::string& replyTo = to.find_first_of("#&") == 0 ? to : from;
	std::stringstream ss;
	ss.imbue(std::locale::classic());
	ss<<time(0)<<" "<<replyTo<<": <"<<from<<"> "
	  <<CleanMessageForDisplay(from, message[1]);
	Log(replyTo, ss.str());
    }
    else if ( message.GetCommand() == "353" && message.size() >= 4)
    {
	// RPL_NAMREPLY

	std::string channel = message[2];

	if ( channel.find_first_of("#&") == 0 )
	{
	    std::stringstream stream(message[3]);

	    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	    while ( stream.good() )
	    {
		std::string nick;
		stream>>nick;
		if ( nick.find_first_of("@+") == 0 )
		{
		    nick.erase(0, 1);
		}
		
		if ( nick.size() > 0 )
		{
		    channelNicks_[channel].insert(nick);
		}
	    }
	}
    }
    else if ( message.GetCommand() == "366" )
    {
	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	appendingChannelNicks_ = false;
    }
    else if ( message.GetCommand() == "PART" && message.size() >= 1 )
    {
	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	const std::string& nick = message.GetPrefix().GetNick();
	const std::string& channel = message[0];
	channelNicks_[channel].erase(nick);
    }
    else if ( message.GetCommand() == "QUIT" )
    {
	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	const std::string& nick = message.GetPrefix().GetNick();
	for(ChannelNicksContainer::iterator channel = channelNicks_.begin();
	    channel != channelNicks_.end();
	    ++channel)
	{
	    channel->second.erase(nick);
	}	    
    }
    else if ( message.GetCommand() == "JOIN" && message.size() >= 1 )
    {
	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	const std::string& nick = message.GetPrefix().GetNick();
	const std::string& channel = message[0];
	channelNicks_[channel].insert(nick);
    }
    else if ( message.GetCommand() == "NICK" && message.size() >= 1 )
    {
	const std::string& oldNick = message.GetPrefix().GetNick();
	const std::string& newNick = message[0];

	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	for(ChannelNicksContainer::iterator channel = channelNicks_.begin();
	    channel != channelNicks_.end();
	    ++channel)
	{
	    channel->second.erase(oldNick);
	    channel->second.insert(newNick);
	}
    }
    else if ( message.GetCommand() == "KICK" && message.size() >= 2 )
    {
	const std::string& channel = message[0];
	const std::string& victim = message[1];

	boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
	ChannelNicksContainer::iterator nicks = channelNicks_.find(channel);
	if ( nicks != channelNicks_.end() )
	{
	    nicks->second.erase(victim);
	}

	if ( victim == GetNick() )
	{
	    // We've been kicked
	    if ( nicks != channelNicks_.end() )
	    {
		// Remove channel, we'll get a new list of nicks if we rejoin
		channelNicks_.erase(channel);
		if ( channels_.find(channel) != channels_.end() )
		{
		    JoinChannel(channel, channels_[channel]);
		}
		else
		{
		    JoinChannel(channel, "");
		}
	    }
	}
    }

    boost::shared_lock<boost::shared_mutex> lock(receiversMutex_);
    // Send message to receivers
    for(ReceiverContainer::iterator i = receivers_.begin();
	i != receivers_.end();)
    {
	if ( ReceiverHandle receiver = i->lock() )
	{
	    (*receiver)(*this, message);
	    ++i;
	}
	else
	{
	    // If a receiver is no longer valid we remove it
	    boost::upgrade_lock<boost::shared_mutex> lock(receiversMutex_);
	    i = receivers_.erase(i);
	}
    }

}

void Server::RegisterSelfAsReceiver()
{
    connectionReceiver_ = connection_.RegisterReceiver(
	boost::bind(&Server::Receive, this, _1, _2));
    onConnectCallback_ = connection_.RegisterOnConnectCallback(
	boost::bind(&Server::OnConnect, this, _1));
}

void Server::Log(const std::string& target,
		 const std::string& text)
{
    boost::lock_guard<boost::mutex> lock(logMutex_);
    std::string logFile = GetLogName(target);

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

void Server::ManageLogStreams()
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
	    std::advance(next, 1),
	    logStreams_.erase(i);
	    i = next;
	}
	else
	{
	    ++i;
	}
    }
}

std::string Server::CleanMessageForDisplay(const std::string& nick,
					   const std::string& message)
{
    std::string result = message;

    const std::string ctcpAction = "\1ACTION";
    std::string::size_type pos = std::string::npos;
    if ( result.find(ctcpAction) == 0 )
    {
	result.replace(0, ctcpAction.size(), "* "+nick);
	if ( (pos=result.rfind("\1")) != std::string::npos )
	{
	    result.erase(pos);
	}
    }
    return result;
}
