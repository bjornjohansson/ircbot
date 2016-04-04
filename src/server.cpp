#include "server.hpp"
#include "message.hpp"
#include "logging/logger.hpp"
#include "exception.hpp"

#include <sstream>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string/trim.hpp>

static void EnsureDirectoryExists(const std::string& directory)
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

Server::Server(const UnicodeString& id,
               const UnicodeString& host,
               unsigned int port,
               const std::string& logDirectory,
               const UnicodeString& nick,
               const UnicodeString& serverPassword)
    : id_(id)
    , host_(host)
    , port_(port)
    , nick_(nick)
    , serverPassword_(serverPassword)
    , logDirectory_(logDirectory)
{
}

Server::~Server()
{
}

const UnicodeString& Server::GetId() const
{
    return id_;
}

const UnicodeString& Server::GetHostName() const
{
    return host_;
}

unsigned int Server::GetPort() const
{
    return port_;
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

void Server::AddChannel(const std::string& channel, const UnicodeString& key)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelsMutex_);
    channels_[channel] = key;
}

void Server::JoinAllChannels()
{
    boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
    for (ChannelKeyMap::iterator channel = channels_.begin();
         channel != channels_.end();
         ++channel)
    {
        JoinChannel(channel->first, channel->second);
    }
}

const UnicodeString* Server::GetChannelKey(const std::string& channel) const
{
    boost::shared_lock<boost::shared_mutex> lock(channelsMutex_);
    ChannelKeyMap::const_iterator channelIt = channels_.find(channel);
    if (channelIt != channels_.end())
    {
        return &channelIt->second;
    }
    return 0;
}

void Server::SetNick(const UnicodeString& nick)
{
    Send("NICK " + AsUtf8(nick));
    boost::upgrade_lock<boost::shared_mutex> lock(nickMutex_);
    nick_ = nick;
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

void Server::RemoveChannelNickChannel(const std::string& channel)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    channelNicks_.erase(channel);
}

void Server::ClearAllChannelNicks(const std::string& channel)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    channelNicks_[channel].clear();
}

void Server::AddChannelNick(const std::string& channel, const std::string& nick)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    channelNicks_[channel].insert(nick);
}

void Server::ChangeChannelNick(const std::string& oldNick, const std::string& newNick)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    for (ChannelNicksContainer::iterator channel = channelNicks_.begin();
         channel != channelNicks_.end();
         ++channel)
    {
        channel->second.erase(oldNick);
        channel->second.insert(newNick);
    }
}

void Server::RemoveChannelNick(const std::string& nick) {
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    for (ChannelNicksContainer::iterator channel = channelNicks_.begin();
         channel != channelNicks_.end();
         ++channel)
    {
        channelNicks_[channel->first].erase(nick);
    }
}

void Server::RemoveChannelNick(const std::string& channel, const std::string& nick)
{
    boost::upgrade_lock<boost::shared_mutex> lock(channelNicksMutex_);
    channelNicks_[channel].erase(nick);
}

const UnicodeString& Server::GetServerPassword() const
{
    return serverPassword_;
}

void Server::NotifyReceiver(const Message& message)
{
    boost::shared_lock<boost::shared_mutex> lock(receiversMutex_);
    // Send message to receivers
    for (ReceiverContainer::iterator i = receivers_.begin();
         i != receivers_.end();)
    {
        if (ReceiverHandle receiver = i->lock())
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

const UnicodeString& Server::GetHost() const
{
    return host_;
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

