#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "socket.hpp"

#include <string>
#include <vector>
#include <list>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>

class ConnectionManager;

const std::size_t RECEIVE_BUFFER_SIZE = 1024;

class Connection
{
private:
    /**
     * This is private because we don't want anyone to create connections.
     * Only ConnectionManager should be doing that.
     * @throw Exception if unable to connect
     */
    Connection(boost::asio::io_service& service,
	       const std::string& host,
	       const unsigned short port);

public:
    typedef boost::function<void (Connection&, 
				  const std::vector<char>&)> Receiver;
    typedef boost::shared_ptr<Receiver> ReceiverHandle;
    typedef boost::function<void (Connection&)> OnConnectCallback;
    typedef boost::shared_ptr<OnConnectCallback> OnConnectHandle;

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

    OnConnectHandle RegisterOnConnectCallback(OnConnectCallback c);

    /**
     * @throw Exception if there's not valid socket
     */
//    int GetSocket() const;

    bool IsTimedOut() const;
    bool IsConnected() const;

private:

    void Connect();

    void OnConnect(const boost::system::error_code& error,
		   boost::asio::ip::tcp::resolver::iterator endpointIt);

    void CreateReceiver();
    void Receive(const boost::system::error_code& error,
		 const std::size_t& bytes);

    friend class ConnectionManager;

    boost::asio::io_service& ioService_;
    boost::asio::ip::tcp::socket socket_;
    boost::array<char, RECEIVE_BUFFER_SIZE> buffer_;

    std::string host_;
    unsigned short port_;
    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    typedef boost::weak_ptr<OnConnectCallback> OnConnectCallbackPtr;
    typedef std::list<OnConnectCallbackPtr> OnConnectCallbackContainer;
    OnConnectCallbackContainer onConnectCallbacks_;
    boost::shared_mutex callbacksMutex_;
    time_t lastReception_;
    bool connected_;
    boost::mutex sendMutex_;
    mutable boost::shared_mutex connectedMutex_;
};

#endif

