#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../thread_safe.hpp"

#include <string>
#include <vector>
#include <list>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

const std::size_t RECEIVE_BUFFER_SIZE = 1024;

class Connection : boost::noncopyable
{
public:
    Connection();
    /**
     * @throw Exception if unable to connect
     */
    Connection(const std::string& host, const unsigned short port);
    virtual ~Connection();

    /**
     * @throw Exception if unable to connect
     */
    void Connect(const std::string& host, const unsigned short port);

    /**
     * @throw Exception if unable to reconnect
     */
    void Reconnect();

    typedef boost::function<void (Connection&, 
				  const std::vector<char>&)> Receiver;
    typedef boost::shared_ptr<Receiver> ReceiverHandle;
    /**
     * Register a function as a receiver, the return value is a handle
     * to the receiver, whenever this handle ceases to exist the connection
     * will no longer send data to the corresponding receiver.
     */
    ReceiverHandle RegisterReceiver(Receiver r);
    void Send(const std::vector<char>& data);

    typedef boost::function<void (Connection&)> OnConnectCallback;
    typedef boost::shared_ptr<OnConnectCallback> OnConnectHandle;

    /**
     * Register a function as a callback for OnConnect events.
     * The return value is a handle to the callback, whenever this handle
     * ceases to exist the connection will no longer notify the callback
     * of OnConnect events.
     */
    OnConnectHandle RegisterOnConnectCallback(OnConnectCallback c);

    bool IsTimedOut() const;
    bool IsConnected() const;

private:
    void OnConnect(const boost::system::error_code& error,
		   boost::asio::ip::tcp::resolver::iterator endpointIt);

    void CreateReceiver();
    void Receive(const boost::system::error_code& error,
		 const std::size_t& bytes);

    void Loop();
    void CheckConnection();

    boost::asio::io_service ioService_;
    boost::asio::ip::tcp::socket socket_;
    boost::mutex socketMutex_;
    boost::array<char, RECEIVE_BUFFER_SIZE> buffer_;

    std::string host_;
    unsigned short port_;
    typedef std::list<boost::weak_ptr<Receiver> > ReceiverContainer;
    ReceiverContainer receivers_;
    boost::shared_mutex receiversMutex_;
    typedef boost::weak_ptr<OnConnectCallback> OnConnectCallbackPtr;
    typedef std::list<OnConnectCallbackPtr> OnConnectCallbackContainer;
    OnConnectCallbackContainer onConnectCallbacks_;
    boost::shared_mutex callbacksMutex_;
    time_t lastReception_;
    thread_safe<bool> connected_;
    boost::mutex sendMutex_;
    std::auto_ptr<boost::thread> thread_;

    bool run_;
    boost::condition_variable workAvailable_;
    boost::mutex workAvailableMutex_;
};

#endif

