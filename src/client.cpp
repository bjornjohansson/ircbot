#include "client.hpp"
#include "exception.hpp"
#include "searchidle.h"
#include "glue/gluemanager.hpp"
#include "irc/ircserver.hpp"
#include "message.hpp"
#include "server.hpp"
#include "lua/lua.hpp"
#include "logging/logger.hpp"

#include <fstream>
#include <sstream>
#include <locale>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string.hpp>

Client::Client(const UnicodeString& config) :
    config_(config), run_(false)
{
    InitLua();
    try
    {
        int numServers = std::distance(config_.GetServersBegin(),
                                       config_.GetServersEnd());
        if (numServers == 0)
        {
            Log << LogLevel::Warning << "No servers configured, nothing to do";
        }
        for (Config::ServerIterator i = config_.GetServersBegin(); i
                != config_.GetServersEnd(); ++i)
        {
            serverSettings_[i->GetId()] = *i;
            Connect(i->GetServerType(),
                    i->GetId(),
                    i->GetHost(),
                    i->GetPort(),
                    i->GetNick(),
                    i->GetPassword());
        }
        run_ = true;
    } catch (Exception& e)
    {
        Log << LogLevel::Error << e.GetMessage();
    }

    try
    {
        namedPipe_.reset(new NamedPipe(config_.GetNamedPipeName()));
        pipeReceiver_ = namedPipe_->RegisterReceiver(boost::bind(
                &Client::ReceivePipeMessage, this, _1));
    } catch (Exception&)
    {
        Log << LogLevel::Error << "Could not open named pipe";
    }
}

void Client::Run()
{
    if (run_)
    {
        boost::unique_lock<boost::mutex> lock(runMutex_);
        runCondition_.wait(lock);
    }
}

void Client::Receive(Server& server, const Message& message)
{
    currentServer_ = server.GetId();
    currentReplyTo_.clear();

    if (!OnPrivMsg(server, message))
    {
        if (server.GetNick().caseCompare(ConvertString(message.GetPrefix().GetNick()), 0) == 0)
        {
            // We do not process messages from ourself
            return;
        }
        boost::shared_lock<boost::shared_mutex> lock(receiverMutex_);
        for (EventReceiverContainer::iterator i = eventReceivers_.begin();
             i != eventReceivers_.end();
             ++i)
        {
            if (EventReceiverHandle receiver = i->lock())
            {
                (*receiver)(server.GetId(), message);
            }
        }
    }
}

void Client::JoinChannel(const std::string& channel,
        const UnicodeString& key, const UnicodeString& serverId)
{
    GetServerFromId(serverId).JoinChannel(channel, key);
}

void Client::Kick(const std::string& user, const UnicodeString& message,
        const std::string& channel, const UnicodeString& serverId)
{
    const std::string& chan = channel.empty() ? currentReplyTo_ : channel;
    GetServerFromId(serverId).Kick(chan, user, message);
}

void Client::ChangeNick(const UnicodeString& nick,
        const UnicodeString& serverId)
{
    GetServerFromId(serverId).ChangeNick(nick);
}

void Client::SendMessage(const UnicodeString& message,
        const std::string& target, const UnicodeString& serverId)
{
    const std::string& to = target.empty() ? currentReplyTo_ : target;
    GetServerFromId(serverId).SendMessage(to, message);
}

Client::EventReceiverHandle Client::RegisterForEvent(EventReceiver receiver)
{
    boost::upgrade_lock<boost::shared_mutex> lock(receiverMutex_);
    EventReceiverHandle handle(new EventReceiver(receiver));
    eventReceivers_.push_back(EventReceiverPtr(handle));
    return handle;
}

const Config& Client::GetConfig() const
{
    return config_;
}

std::string Client::GetLogName(const std::string& target,
        const UnicodeString& serverId) const
{
    const std::string& to = target.empty() ? currentReplyTo_ : target;
    return GetServerFromId(serverId).GetLogName(to);
}

UnicodeString Client::GetLastLine(const std::string& nick, long& timestamp,
        const std::string& channel, const UnicodeString& serverId) const
{
    const std::string& to = channel.empty() ? currentReplyTo_ : channel;
    std::string logName = GetServerFromId(serverId).GetLogName(to);

    std::vector<char> buffer(1024, 0);

    int res = searchidle(logName.c_str(), nick.c_str(),
            &timestamp, &buffer[0], buffer.size());

    UnicodeString result;
    if (res == 0)
    {
      result = ConvertString(&buffer[0]);
    }
    else
    {
        timestamp = -1;
    }

    return result;
}

const UnicodeString& Client::GetNick(const UnicodeString& serverId)
{
    return GetServerFromId(serverId).GetNick();
}

const std::set<std::string>&
Client::GetChannelNicks(const std::string& channel,
        const UnicodeString& serverId)
{
    const std::string& to = channel.empty() ? currentReplyTo_ : channel;
    return GetServerFromId(serverId).GetChannelNicks(to);
}

Server& Client::GetServerFromId(const UnicodeString& id)
{
    const UnicodeString& serverId = id.isEmpty() ? currentServer_ : id;

    boost::shared_lock<boost::shared_mutex> lock(serverMutex_);
    ServerHandleMap::iterator server = servers_.find(serverId);
    if (server != servers_.end())
    {
        return *server->second.first;
    }
    else
    {
        throw Exception(__FILE__, __LINE__, "No server named '" + id + "'");
    }
}

const Server& Client::GetServerFromId(const UnicodeString& id) const
{
    const UnicodeString& serverId = id.isEmpty() ? currentServer_ : id;

    boost::shared_lock<boost::shared_mutex> lock(serverMutex_);
    ServerHandleMap::const_iterator server = servers_.find(serverId);
    if (server != servers_.end())
    {
        return *server->second.first;
    }
    else
    {
        throw Exception(__FILE__, __LINE__, "No server named '" + id + "'");
    }
}

Server& Client::Connect(Config::Server::Type serverType,
                        const UnicodeString& id,
                        const UnicodeString& host,
                        unsigned int port,
                        const UnicodeString& nick,
                        const UnicodeString& password)
{
    boost::upgrade_lock<boost::shared_mutex> lock(serverMutex_);

    ServerPtr server = CreateServer(serverType,
                                    id,
                                    host,
                                    port,
                                    nick,
                                    password);
    Server::ReceiverHandle handle =
        server->RegisterReceiver(boost::bind(&Client::Receive, this, _1, _2));

    std::pair<ServerHandleMap::iterator, bool> inserted =
        servers_.insert(ServerHandleMap::value_type(id,
                                                    ServerAndHandle(server,
                                                                    handle)));

    server->Connect();
    ServerAndHandle serverAndHandle = inserted.first->second;
    return *serverAndHandle.first;
}

Client::ServerPtr Client::CreateServer(Config::Server::Type serverType,
                                       const UnicodeString& id,
                                       const UnicodeString& host,
                                       unsigned int port,
                                       const UnicodeString& nick,
                                       const UnicodeString& password)
{
    std::string logDirectory = AsUtf8(config_.GetLogsDirectory());
    if (logDirectory.rfind('/') != logDirectory.size() - 1)
    {
        logDirectory += '/';
    }
    logDirectory += AsUtf8(id) + '/';

    ServerPtr server;
    switch (serverType)
    {
    case Config::Server::IrcServer:
        Log << LogLevel::Info << "Creating IRC server for " << host << ":" << port;
        server.reset(new Irc::IrcServer(id,
                                        host,
                                        port,
                                        logDirectory,
                                        nick,
                                        password));
        break;
    case Config::Server::MatrixServer:
    default:
        throw Exception(__FILE__, __LINE__, "Invalid server type");
    }
    for (Config::ServerIterator serverIt = config_.GetServersBegin();
         serverIt != config_.GetServersEnd();
         ++serverIt)
    {
        if (id == serverIt->GetId())
        {
            for (Config::Server::ChannelIterator channelIt = serverIt->GetChannelsBegin();
                 channelIt != serverIt->GetChannelsEnd();
                 ++channelIt)
            {
                const std::string& channel = channelIt->first;
                const UnicodeString& key = channelIt->second;
                server->AddChannel(channel, key);
            }
        }
    }
    return server;
}

bool Client::OnPrivMsg(Server& server, const Message& message)
{
    std::string fromNick = message.GetPrefix().GetNick();
    UnicodeString to = ConvertString(message[0]);
    UnicodeString text = ConvertString(message[1]);

    if (text == (server.GetNick() + AsUnicode(": reload")) || (to
            == server.GetNick() && text == AsUnicode("reload")))
    {
        InitLua();
        return true;
    }
    else if (text == (server.GetNick() + ": quit") || (to == server.GetNick()
            && text == "quit"))
    {
        runCondition_.notify_all();
        return true;
    }
    return false;
}

void Client::ReceivePipeMessage(const std::string& line)
{
    std::stringstream ss(line);
    std::string command, server, channel, message;
    ss >> command >> server >> channel;
    if (ss.peek() == ' ')
    {
        ss.get();
    }
    std::getline(ss, message);

    try
    {
        if (boost::iequals(command, "say"))
        {
            SendMessage(ConvertString(message), channel, AsUnicode(server));
        }
    } catch (Exception& e)
    {
        Log << LogLevel::Error << "Named pipe failed to send message: "
                << e.GetMessage();
    }
}

void Client::LogMessage(Server& server, const UnicodeString& target,
        const UnicodeString& text)
{
    UnicodeString logFile = config_.GetLogsDirectory();
    if (logFile.lastIndexOf(AsUnicode("/")) != logFile.length() - 1)
    {
        logFile += "/";
    }
    logFile += server.GetHostName() + "/";
    logFile += target;

    time_t currentTime = time(0);
    LogStreamMap::iterator stream = logStreams_.find(logFile);
    if (stream == logStreams_.end())
    {
        Log << LogLevel::Debug << "Creating '" << logFile << "' log stream";
        OfstreamPtr
                newStream(new std::ofstream(AsUtf8(logFile).c_str(), std::ios::app));
        logStreams_[logFile] = OfstreamAndTimestamp(newStream, currentTime);
        stream = logStreams_.find(logFile);
    }
    if (stream != logStreams_.end())
    {
        (*stream->second.first) << AsUtf8(text) << std::endl;
        stream->second.second = currentTime;
    }

}

void Client::ManageLogStreams()
{
    time_t currentTime = time(0);
    for (LogStreamMap::iterator i = logStreams_.begin(); i != logStreams_.end();)
    {
        // Only keep logstreams alive for a certain amount of time
        if (i->second.second + 30 * 60 < currentTime)
        {
            Log << LogLevel::Debug << "Closing '" << i->first << "' log stream";
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
