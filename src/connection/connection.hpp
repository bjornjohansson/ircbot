#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "socket.hpp"

#include <string>
#include <vector>
#include <list>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class Connection
{
public:
    typedef boost::function<void (Connection&, 
				  const std::vector<char>&)> Receiver;
    typedef boost::shared_ptr<Receiver> ReceiverHandle;
    typedef boost::function<void (Connection&)> OnConnectCallback;
    typedef boost::shared_ptr<OnConnectCallback> OnConnectHandle;

    /**
     * @throw Exception if unable to connect
     */
    Connection(const std::string& host, const unsigned short port);

    /**
     * @throw Exception if unable to reconnect
     */
    void Reconnect();

    /**
     * Register a function as a receiver, the return value is a handle
     * to the receiver, whenever this handle ceases to exist the connection
     * will no longer send data to the corresponding receiver.
     */
    ReceiverHandle RegisterReceiver(Receiver r);
    void Send(const std::vector<char>& data);
    void Receive();

    OnConnectHandle RegisterOnConnectCallback(OnConnectCallback c);

    /**
     * @throw Exception if there's not valid socket
     */
    int GetSocket() const;

    bool IsTimedOut() const;
    bool IsConnected() const;

private:
    void Connect();

    boost::shared_ptr<Socket> socket_;
    std::string host_;
    unsigned short port_;
    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    typedef boost::weak_ptr<OnConnectCallback> OnConnectCallbackPtr;
    typedef std::list<OnConnectCallbackPtr> OnConnectCallbackContainer;
    OnConnectCallbackContainer onConnectCallbacks_;
    time_t lastReception_;
    bool connected_;
};

#endif

