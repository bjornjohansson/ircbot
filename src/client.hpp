#pragma once

#include "irc/server.fwd.hpp"
#include "irc/prefix.fwd.hpp"
#include "irc/message.fwd.hpp"
#include "irc/command.hpp"
#include "config.hpp"
#include "lua/lua.fwd.hpp"
#include "connection/namedpipe.hpp"

#include <string>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class Client
{
public:
    Client(const std::string& config);

    void Run();

    void Receive(Server& server, const Irc::Message& message);

    /**
     * @throw Exception if no matching server found
     */
    void JoinChannel(const std::string& channel,
		     const std::string& key = std::string(),
		     const std::string& serverId = std::string());

    void Kick(const std::string& user,
	      const std::string& message,
	      const std::string& channel = std::string(),
	      const std::string& server = std::string());

    /**
     * @throw Exception if no matching server found
     */
    void ChangeNick(const std::string& nick,
		    const std::string& serverId = std::string());
    /**
     * @throw Exception if no matching server found
     */
    void SendMessage(const std::string& message,
		     const std::string& target = std::string(),
		     const std::string& serverId = std::string());

    /**
     * @throw Exception if no matching server found
     */
    std::string GetLogName(const std::string& target = std::string(),
			   const std::string& serverId = std::string()) const;
    /**
     * @throw Exception if no matching server found
     * @param timestamp becomes -1 if no last line can be found
     */
    std::string GetLastLine(const std::string& nick,
			    long& timestamp,
			    const std::string& channel = std::string(),
			    const std::string& serverId = std::string()) const;
    /**
     * @throw Exception if no matching server found
     */
    const std::string& GetNick(const std::string& serverId = std::string());

    /**
     * @throw Exception if no matching server or channel found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel = std::string(),
						 const std::string& serverid = std::string());

    // void (server, message)
    typedef boost::function<void (const std::string&,
				  const Irc::Message&)> EventReceiver;
    typedef boost::shared_ptr<EventReceiver> EventReceiverHandle;
    EventReceiverHandle RegisterForEvent(const Irc::Command::Command& event,
					 EventReceiver receiver);

    const Config& GetConfig() const;

private:

    /**
     * @throw Exception if no matching server found
     */
    Server& GetServerFromId(const std::string& id);

    /**
     * @throw Exception if no matching server found
     */
    const Server& GetServerFromId(const std::string& id) const;

    /**
     * @throw Exception if unable to connect
     */
    Server& Connect(const std::string& id,
		    const std::string& host,
		    unsigned int port,
		    const std::string& nick);

    /** @return true if the message should block continued processing */
    bool OnPrivMsg(Server& server, const Irc::Message& message);

    void ReceivePipeMessage(const std::string& line);

    void LogMessage(Server& server,
		    const std::string& target,
		    const std::string& text);

    void ManageLogStreams();

    void InitLua();

    typedef boost::shared_ptr<Server> ServerPtr;
    typedef std::pair<ServerPtr, ServerReceiverHandle> ServerAndHandle;
    typedef std::map<std::string,ServerAndHandle> ServerHandleMap;
    ServerHandleMap servers_;
    mutable boost::shared_mutex serverMutex_;
    Config config_;

    typedef std::map<std::string, Config::Server> ServerIdMap;
    ServerIdMap serverSettings_;

    typedef boost::shared_ptr<std::ofstream> OfstreamPtr;
    typedef std::pair<OfstreamPtr,time_t> OfstreamAndTimestamp;
    typedef std::map<std::string, OfstreamAndTimestamp> LogStreamMap;
    LogStreamMap logStreams_;

    typedef boost::weak_ptr<EventReceiver> EventReceiverPtr;
    typedef std::list<EventReceiverPtr> EventReceiverContainer;
    typedef boost::unordered_map<Irc::Command::Command,EventReceiverContainer>
        EventReceiverMap;
    EventReceiverMap eventReceivers_;
    boost::shared_mutex receiverMutex_;

    boost::shared_ptr<Lua> lua_;
    boost::mutex runMutex_;
    boost::condition_variable runCondition_;

    boost::shared_ptr<NamedPipe> namedPipe_;
    NamedPipe::ReceiverHandle pipeReceiver_;

    std::string currentServer_;
    std::string currentReplyTo_;
    bool run_;
};
