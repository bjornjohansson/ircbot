#ifndef SERVER_HPP
#define SERVER_HPP

#include "server.fwd.hpp"

#include "../connection/connection.hpp"
#include "message.fwd.hpp"

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
    Server(const std::string& id,
	   const std::string& host,
	   unsigned int port,
	   const std::string& logsDirectory,
	   const std::string& nick);
    virtual ~Server();

    void Send(const std::string& data);

    const std::string& GetId() const;
    const std::string& GetHostName() const;
    const std::string& GetNick() const;

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
    void JoinChannel(const std::string& channel, const std::string& key);
    void ChangeNick(const std::string& nick);
    void SendUserString(const std::string& user, const std::string name);
    void SendMessage(const std::string& target, const std::string& message);
    void Kick(const std::string& channel,
	      const std::string& user,
	      const std::string& message);

    /**
     * @throw Exception if channel not found
     */
    const std::set<std::string>& GetChannelNicks(const std::string& channel) const;
    

private:
    void Receive(Connection& connection, const std::vector<char>& data);
    void OnConnect(Connection& connection);

    void OnText(const std::string& text);

    void RegisterSelfAsReceiver();

    void Log(const std::string& target,
	     const std::string& text);

    void ManageLogStreams();

    std::string CleanMessageForDisplay(const std::string& nick,
				       const std::string& message);

    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    boost::shared_mutex receiversMutex_;

    Connection connection_;

    Connection::ReceiverHandle connectionReceiver_;
    Connection::OnConnectHandle onConnectCallback_;

    std::string receiveBuffer_;
    boost::mutex callbackMutex_;

    std::string id_;
    std::string host_;
    std::string logDirectory_;
    std::string nick_;
    mutable boost::shared_mutex nickMutex_;

    typedef boost::shared_ptr<std::ofstream> OfstreamPtr;
    typedef std::pair<OfstreamPtr,time_t> OfstreamAndTimestamp;
    typedef std::map<std::string, OfstreamAndTimestamp> LogStreamMap;
    LogStreamMap logStreams_;
    boost::mutex logMutex_;

    typedef std::map<std::string, std::string> ChannelKeyMap;
    ChannelKeyMap channels_;
    mutable boost::shared_mutex channelsMutex_;

    typedef std::map<std::string,std::set<std::string> > ChannelNicksContainer;
    ChannelNicksContainer channelNicks_;
    bool appendingChannelNicks_;
    boost::shared_mutex channelNicksMutex_;
};


#endif
