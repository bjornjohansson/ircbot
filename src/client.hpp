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

#include <unicode/unistr.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class Client
{
public:
    Client(const UnicodeString& config);

    void Run();

    void Receive(Server& server, const Irc::Message& message);

    /**
     * @throw Exception if no matching server found
     */
    void JoinChannel(const std::string& channel,
		     const UnicodeString& key = UnicodeString(),
		     const UnicodeString& serverId = UnicodeString());

    void Kick(const std::string& user,
	      const UnicodeString& message,
	      const std::string& channel = std::string(),
	      const UnicodeString& server = UnicodeString());

    /**
     * @throw Exception if no matching server found
     */
    void ChangeNick(const UnicodeString& nick,
		    const UnicodeString& serverId = UnicodeString());
    /**
     * @throw Exception if no matching server found
     */
    void SendMessage(const UnicodeString& message,
		     const std::string& target = std::string(),
		     const UnicodeString& serverId = UnicodeString());

    /**
     * @throw Exception if no matching server found
     */
    std::string GetLogName(const std::string& target = std::string(),
			   const UnicodeString& serverId = UnicodeString()) const;
    /**
     * @throw Exception if no matching server found
     * @param timestamp becomes -1 if no last line can be found
     */
    UnicodeString GetLastLine(const std::string& nick,
			    long& timestamp,
			    const std::string& channel = std::string(),
			    const UnicodeString& serverId = UnicodeString()) const;
    /**
     * @throw Exception if no matching server found
     */
    const UnicodeString& GetNick(const UnicodeString& serverId = UnicodeString());

    /**
     * @throw Exception if no matching server or channel found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel = std::string(),
						 const UnicodeString& serverid = UnicodeString());

    // void (server, message)
    typedef boost::function<void (const UnicodeString&,
				  const Irc::Message&)> EventReceiver;
    typedef boost::shared_ptr<EventReceiver> EventReceiverHandle;
    EventReceiverHandle RegisterForEvent(const Irc::Command::Command& event,
					 EventReceiver receiver);

    const Config& GetConfig() const;

private:

    /**
     * @throw Exception if no matching server found
     */
    Server& GetServerFromId(const UnicodeString& id);

    /**
     * @throw Exception if no matching server found
     */
    const Server& GetServerFromId(const UnicodeString& id) const;

    /**
     * @throw Exception if unable to connect
     */
    Server& Connect(const UnicodeString& id,
		    const UnicodeString& host,
		    unsigned int port,
		    const UnicodeString& nick,
		    const UnicodeString& password = UnicodeString());

    /** @return true if the message should block continued processing */
    bool OnPrivMsg(Server& server, const Irc::Message& message);

    void ReceivePipeMessage(const std::string& line);

    void LogMessage(Server& server,
		    const UnicodeString& target,
		    const UnicodeString& text);

    void ManageLogStreams();

    void InitLua();

    typedef boost::shared_ptr<Server> ServerPtr;
    typedef std::pair<ServerPtr, ServerReceiverHandle> ServerAndHandle;
    typedef std::map<UnicodeString,ServerAndHandle> ServerHandleMap;
    ServerHandleMap servers_;
    mutable boost::shared_mutex serverMutex_;
    Config config_;

    typedef std::map<UnicodeString, Config::Server> ServerIdMap;
    ServerIdMap serverSettings_;

    typedef boost::shared_ptr<std::ofstream> OfstreamPtr;
    typedef std::pair<OfstreamPtr,time_t> OfstreamAndTimestamp;
    typedef std::map<UnicodeString, OfstreamAndTimestamp> LogStreamMap;
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

    UnicodeString currentServer_;
    std::string currentReplyTo_;
    bool run_;
};
