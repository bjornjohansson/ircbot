#include "server.hpp"
#include "message.hpp"
#include "../logging/logger.hpp"
#include "../exception.hpp"

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
	} catch (boost::filesystem::filesystem_error&)
	{
		Log << LogLevel::Error << "Could not create directories '" << directory
				<< "'";
	}
}

Server::Server(const UnicodeString& id, const UnicodeString& host,
		unsigned int port, const std::string& logDirectory,
		const UnicodeString& nick, const UnicodeString& serverPassword) :
	id_(id), host_(host), logDirectory_(logDirectory), nick_(nick),
			serverPassword_(serverPassword), appendingChannelNicks_(false)
{
	RegisterSelfAsReceiver();
	connection_.Connect(host, port);
}

Server::~Server()
{
	// Do not destroy this class while it's performing callbacks
	boost::lock_guard<boost::mutex> lock(callbackMutex_);
}

void Server::Send(const std::string& data)
{
	std::vector<char> buffer;
	std::copy(data.begin(), data.end(), std::back_inserter(buffer));
	buffer.push_back('\r');
	buffer.push_back('\n');
	try
	{
		connection_.Send(buffer);
	} catch (Exception& e)
	{
		Log << LogLevel::Error << e.GetMessage();
	}
	Log << LogLevel::Debug << host_ << "> " << data;
}

void Server::Receive(Connection& connection, const std::vector<char>& data)
{
	boost::lock_guard<boost::mutex> lock(callbackMutex_);
	// Copy everything but '\r' as some irc servers don't comply
	std::remove_copy(data.begin(), data.end(), std::back_inserter(
			receiveBuffer_), '\r');

	for (std::string::size_type pos = receiveBuffer_.find('\n');
	     pos != std::string::npos;
	     pos = receiveBuffer_.find('\n'))
	{
		OnText(receiveBuffer_.substr(0, pos));
		receiveBuffer_.erase(0, pos + 1);
	}
}

void Server::OnConnect(Connection& connection)
{
	if (serverPassword_.length() > 0)
	{
		SendPassString(serverPassword_);
	}
	ChangeNick(nick_);
	SendUserString(nick_, nick_);

	boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
	for (ChannelKeyMap::iterator channel = channels_.begin(); channel
			!= channels_.end(); ++channel)
	{
		JoinChannel(channel->first, channel->second);
	}
}

const UnicodeString& Server::GetId() const
{
	return id_;
}

const UnicodeString& Server::GetHostName() const
{
	return host_;
}

const UnicodeString& Server::GetNick() const
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
	if (logFile.rfind("/") != logFile.size() - 1)
	{
		logFile += "/";
	}
	EnsureDirectoryExists(logFile);
	logFile += target;
	return logFile;
}

void Server::JoinChannel(const std::string& channel, const UnicodeString& key)
{
	boost::upgrade_lock<boost::shared_mutex> lock(channelsMutex_);
	channels_[channel] = key;
	if (connection_.IsConnected())
	{
		if (key.length() > 0)
		{
			Send("JOIN " + channel + " " + AsUtf8(key));
		}
		else
		{
			Send("JOIN " + channel);
		}
	}
}

void Server::ChangeNick(const UnicodeString& nick)
{
	Send("NICK " + AsUtf8(nick));
	boost::upgrade_lock<boost::shared_mutex> lock(nickMutex_);
	nick_ = nick;
}

void Server::SendPassString(const UnicodeString& password)
{
	if (password.indexOf(' ') != -1)
	{
		Send("PASS :" + AsUtf8(password));
	}
	else
	{
		Send("PASS " + AsUtf8(password));
	}
}

void Server::SendUserString(const UnicodeString& user, const UnicodeString& name)
{
	Send("USER " + AsUtf8(user) + " localhost "
			+ AsUtf8(GetHostName()) + " :" + AsUtf8(name));
}

void Server::SendMessage(const std::string& target, const UnicodeString& message)
{
	std::string scrubbedMessage = AsUtf8(message);
	std::string::size_type pos = std::string::npos;
	if ((pos = scrubbedMessage.find_first_of("\n\r")) != std::string::npos)
	{
		scrubbedMessage = scrubbedMessage.substr(0, pos);
	}

	if (scrubbedMessage.size() > 0)
	{
		Send("PRIVMSG " + target + " :" + scrubbedMessage);

		scrubbedMessage = CleanMessageForDisplay(AsUtf8(GetNick()), scrubbedMessage);

		std::stringstream ss;
		ss.imbue(std::locale::classic());
		ss << time(0) << " " << target << ": <" << AsUtf8(GetNick()) << "> "
				<< scrubbedMessage;
		LogMessage(target, ss.str());
	}
}

void Server::Kick(const std::string& channel, const std::string& user,
		const UnicodeString& message)
{
	Send(std::string("KICK ") + channel + std::string(" ") + user
			+ std::string(" :") + (message.isEmpty() ? "" : AsUtf8(message)));
}

const std::set<std::string>&
Server::GetChannelNicks(const std::string& channel) const
{
	boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
	ChannelNicksContainer::const_iterator channelNicks = channelNicks_.find(
			channel);
	if (channelNicks != channelNicks_.end())
	{
		return channelNicks->second;
	}
	throw Exception(__FILE__, __LINE__, ConvertString(
			"Could not get nicks for channel '" + channel + "'"));
}

template<class T>
bool IsIrcNameStatus(const T& v)
{
	return v == '@' || v == '+';
}

void Server::OnText(const std::string& text)
{
	Log << LogLevel::Debug << host_ << "< " << text;
	Irc::Message message(text);

	// Log messages
	if (message.GetCommand() == Irc::Command::PRIVMSG && message.size() >= 2)
	{
		const std::string& from = message.GetPrefix().GetNick();
		const std::string& to = message[0];
		const std::string& replyTo = to.find_first_of("#&") == 0 ? to : from;
		std::stringstream ss;
		ss.imbue(std::locale::classic());
		ss << time(0) << " " << replyTo << ": <" << from << "> "
				<< CleanMessageForDisplay(from, message[1]);
		LogMessage(replyTo, ss.str());
	}
	else if (message.GetCommand() == Irc::Command::RPL_NAMREPLY
			&& message.size() >= 4)
	{
		std::string channel = message[2];

		if (channel.find_first_of("#&") == 0)
		{
			if (!appendingChannelNicks_)
			{
				channelNicks_[channel].clear();
				appendingChannelNicks_ = true;
			}

			std::stringstream stream(message[3]);

			boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
			while (stream.good())
			{
				std::string nick;
				stream >> nick;
				if (nick.find_first_of("@+") == 0)
				{
					nick.erase(0, 1);
				}

				if (nick.size() > 0)
				{
					channelNicks_[channel].insert(nick);
				}
			}
		}
	}
	else if (message.GetCommand() == Irc::Command::RPL_ENDOFNAMES)
	{
		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		appendingChannelNicks_ = false;
	}
	else if (message.GetCommand() == Irc::Command::PART && message.size() >= 1)
	{
		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		const std::string& nick = message.GetPrefix().GetNick();
		const std::string& channel = message[0];
		channelNicks_[channel].erase(nick);
	}
	else if (message.GetCommand() == Irc::Command::QUIT)
	{
		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		const std::string& nick = message.GetPrefix().GetNick();
		for (ChannelNicksContainer::iterator channel = channelNicks_.begin(); channel
				!= channelNicks_.end(); ++channel)
		{
			channel->second.erase(nick);
		}
	}
	else if (message.GetCommand() == Irc::Command::JOIN && message.size() >= 1)
	{
		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		const std::string& nick = message.GetPrefix().GetNick();
		const std::string& channel = message[0];
		channelNicks_[channel].insert(nick);
	}
	else if (message.GetCommand() == Irc::Command::NICK && message.size() >= 1)
	{
		const std::string& oldNick = message.GetPrefix().GetNick();
		const std::string& newNick = message[0];

		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		for (ChannelNicksContainer::iterator channel = channelNicks_.begin(); channel
				!= channelNicks_.end(); ++channel)
		{
			channel->second.erase(oldNick);
			channel->second.insert(newNick);
		}
	}
	else if (message.GetCommand() == Irc::Command::KICK && message.size() >= 2)
	{
		const std::string& channel = message[0];
		const std::string& victim = message[1];

		boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
		ChannelNicksContainer::iterator nicks = channelNicks_.find(channel);
		if (nicks != channelNicks_.end())
		{
			nicks->second.erase(victim);
		}

		if (victim == AsUtf8(GetNick()))
		{
			// We've been kicked
			if (nicks != channelNicks_.end())
			{
				// Remove channel, we'll get a new list of nicks if we rejoin
				channelNicks_.erase(channel);
				if (channels_.find(channel) != channels_.end())
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
	for (ReceiverContainer::iterator i = receivers_.begin(); i
			!= receivers_.end();)
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
	connectionReceiver_ = connection_.RegisterReceiver(boost::bind(
			&Server::Receive, this, _1, _2));
	onConnectCallback_ = connection_.RegisterOnConnectCallback(boost::bind(
			&Server::OnConnect, this, _1));
}

void Server::LogMessage(const std::string& target, const std::string& text)
{
	boost::lock_guard<boost::mutex> lock(logMutex_);
	std::string logFile = GetLogName(target);

	time_t currentTime = time(0);
	LogStreamMap::iterator stream = logStreams_.find(logFile);
	if (stream == logStreams_.end())
	{
		Log << LogLevel::Debug << "Creating '" << logFile << "' log stream";
		OfstreamPtr
				newStream(new std::ofstream(logFile.c_str(), std::ios::app));
		logStreams_[logFile] = OfstreamAndTimestamp(newStream, currentTime);
		stream = logStreams_.find(logFile);
	}
	if (stream != logStreams_.end())
	{
		(*stream->second.first) << text << std::endl;
		stream->second.second = currentTime;
	}
}

void Server::ManageLogStreams()
{
	time_t currentTime = time(0);
	for (LogStreamMap::iterator i = logStreams_.begin(); i != logStreams_.end();)
	{
		// Only keep logstreams alive for a certain amount of time
		if (i->second.second + 30 * 60 < currentTime)
		{
			Log << LogLevel::Debug << "Closing '" << i->first << "' log stream";
			LogStreamMap::iterator next = i;
			std::advance(next, 1), logStreams_.erase(i);
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
	if (result.find(ctcpAction) == 0)
	{
		result.replace(0, ctcpAction.size(), "* " + nick);
		if ((pos = result.rfind("\1")) != std::string::npos)
		{
			result.erase(pos);
		}
	}
	return result;
}
