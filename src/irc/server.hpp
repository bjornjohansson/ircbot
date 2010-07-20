#ifndef SERVER_HPP
#define SERVER_HPP

#include "server.fwd.hpp"

#include "../connection/connection.hpp"
#include "message.fwd.hpp"

#include <charsetdetector.hpp>
#include <unicode/unistr.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

class Server : boost::noncopyable
{
public:
    /**
     * @throw Exception if unable to connect
     */
    Server(const UnicodeString& id,
	   const UnicodeString& host,
	   unsigned int port,
	   const std::string& logsDirectory,
	   const UnicodeString& nick,
	   const UnicodeString& serverPassword = UnicodeString());
    virtual ~Server();

    void Send(const std::string& data);

    const UnicodeString& GetId() const;
    const UnicodeString& GetHostName() const;
    const UnicodeString& GetNick() const;

    typedef ServerReceiver Receiver;
    typedef ServerReceiverHandle ReceiverHandle;

    /**
     * Register a function as a receiver, the return value is a handle
     * to the receiver, whenever this handle ceases to exist the server
     * will no longer send data to the corresponding receiver.
     */
    ReceiverHandle RegisterReceiver(Receiver receiver);

    std::string GetLogName(const std::string& target) const;

    // Irc methods
    void JoinChannel(const std::string& channel, const UnicodeString& key);
    void ChangeNick(const UnicodeString& nick);
    void SendPassString(const UnicodeString& password);
    void SendUserString(const UnicodeString& user, const UnicodeString& name);
    void SendMessage(const std::string& target, const UnicodeString& message);
    void Kick(const std::string& channel,
	      const std::string& user,
	      const UnicodeString& message);

    /**
     * @throw Exception if channel not found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel) const;
    

private:
    void Receive(Connection& connection, const std::vector<char>& data);
    void OnConnect(Connection& connection);

    void OnText(const std::string& text);

    void RegisterSelfAsReceiver();

    void LogMessage(const std::string& target,
		    const std::string& text);

    void ManageLogStreams();

    std::string CleanMessageForDisplay(const std::string& nick,
                                       const std::string& message,
                                       bool isCtcp);

    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    boost::shared_mutex receiversMutex_;

    Connection connection_;

    Connection::ReceiverHandle connectionReceiver_;
    Connection::OnConnectHandle onConnectCallback_;

    std::string receiveBuffer_;
    boost::mutex callbackMutex_;

    UnicodeString id_;
    UnicodeString host_;
    std::string logDirectory_;
    UnicodeString nick_;
    UnicodeString serverPassword_;
    mutable boost::shared_mutex nickMutex_;

    typedef boost::shared_ptr<std::ofstream> OfstreamPtr;
    typedef std::pair<OfstreamPtr,time_t> OfstreamAndTimestamp;
    typedef std::map<std::string, OfstreamAndTimestamp> LogStreamMap;
    LogStreamMap logStreams_;
    boost::mutex logMutex_;

    typedef std::map<std::string, UnicodeString> ChannelKeyMap;
    ChannelKeyMap channels_;
    mutable boost::shared_mutex channelsMutex_;

    typedef std::map<std::string,std::set<std::string> > ChannelNicksContainer;
    ChannelNicksContainer channelNicks_;
    bool appendingChannelNicks_;
    boost::shared_mutex channelNicksMutex_;

    CharsetDetector detector_;
};


#endif
