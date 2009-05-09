#include "connection.hpp"
#include "../exception.hpp"
#include "../error.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#ifdef DEBUG
#include <iostream>
#endif

#include <boost/weak_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

const time_t CONNECTION_TIMEOUT_SECONDS = 300;
// Number of seconds before attempting a new reconnect
const int RECONNECT_TIMEOUT = 30;

Connection::Connection()
    : socket_(ioService_)
    , timer_(ioService_)
    , port_(0)
    , lastReception_(0)
    , connected_(false)
    , run_(true)
      
{
    thread_.reset(new boost::thread(boost::bind(&Connection::Loop, this)));
}

Connection::Connection(const std::string& host, const unsigned short port)
    : socket_(ioService_)
    , timer_(ioService_)
    , port_(0)
    , lastReception_(0)
    , connected_(false)
    , run_(true)
{
    thread_.reset(new boost::thread(boost::bind(&Connection::Loop, this)));
    Connect(host, port);
}

Connection::~Connection()
{
    {
	boost::lock_guard<boost::mutex> lock(socketMutex_);
	boost::system::error_code error;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
	socket_.close(error);
    }
    {
	boost::unique_lock<boost::mutex> lock(workAvailableMutex_);
	run_ = false;
	workAvailable_.notify_all();
	// Wait for the loop to signal that it's done and then join with that thread to make
	// absolutely sure it finished in an orderly manner.
	workAvailable_.wait(lock);
	thread_->join();
    }
}

void Connection::Connect(const std::string& host, const unsigned short port)
{
    try
    {
	host_ = host;
	port_ = port;

	boost::system::error_code error;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
	socket_.close(error);

	// Convert port number to string and remove all non-digits from the
	// string. Certain locales will insert non-digits in numeric strings
	std::string portString = boost::erase_all_regex_copy(
	    boost::lexical_cast<std::string>(port_),
	    boost::regex("[^[:digit:]]"));
	std::clog<<"Connecting to "<<host_<<":"<<portString<<"..."<<std::endl;
	// Create a resolver and a query for the supplied host and port
	using namespace boost::asio::ip;
	tcp::resolver resolver(ioService_);
	tcp::resolver::query query(host_, portString);

	tcp::resolver::iterator endpointIt = resolver.resolve(query);
	tcp::endpoint endpoint = *endpointIt;
	socket_.async_connect(endpoint,
			      boost::bind(&Connection::OnConnect, this,
					  boost::asio::placeholders::error,
					  ++endpointIt));
	workAvailable_.notify_all();
    }
    catch ( boost::bad_lexical_cast& e )
    {
	throw Exception(__FILE__, __LINE__, e.what());
    }
    catch ( std::exception& e )
    {
	throw Exception(__FILE__, __LINE__, e.what());
    }
}

void Connection::Reconnect()
{
    assert(port_ != 0 && !host_.empty());
    Connect(host_, port_);
}

Connection::ReceiverHandle Connection::RegisterReceiver(Receiver r)
{
    ReceiverHandle handle(new Receiver(r));
    boost::upgrade_lock<boost::shared_mutex> lock(receiversMutex_);
    receivers_.push_back(boost::weak_ptr<Receiver>(handle));
    return handle;
}

void Connection::Send(const std::vector<char>& data)
{
    boost::lock_guard<boost::mutex> lock(sendMutex_);
    if ( socket_.is_open() && *connected_ )
    {
	try
	{
	    std::size_t bytes = boost::asio::write(socket_,
						   boost::asio::buffer(data));
	    if ( bytes != data.size() )
	    {
		std::cerr<<"Unable to write on connection"<<std::endl;
		throw Exception(__FILE__, __LINE__,
				"Unable to send the correct amount of data");
	    }
	}
	catch ( boost::system::system_error& e )
	{
	    throw Exception(__FILE__, __LINE__, e.what());
	}
    }
    else
    {
	throw Exception(__FILE__, __LINE__,
			"Attempting to send on invalid connection");
    }
}

Connection::OnConnectHandle
Connection::RegisterOnConnectCallback(OnConnectCallback c)
{
    OnConnectHandle handle(new OnConnectCallback(c));
    boost::upgrade_lock<boost::shared_mutex> lock(callbacksMutex_);
    onConnectCallbacks_.push_back(OnConnectCallbackPtr(handle));
    return handle;
}


bool Connection::IsTimedOut() const
{
    return lastReception_+CONNECTION_TIMEOUT_SECONDS <= time(0);
}

bool Connection::IsConnected() const
{
    return *connected_;
}

void Connection::OnConnect(const boost::system::error_code& error,
			   boost::asio::ip::tcp::resolver::iterator endpointIt)
{
    if ( !error )
    {
	connected_ = true;
	lastReception_ = time(0);

	boost::shared_lock<boost::shared_mutex> lock(callbacksMutex_);
	// Notify all OnConnect callbacks that the connection was successful
	OnConnectCallbackContainer::iterator callback =
	    onConnectCallbacks_.begin();
	while ( callback != onConnectCallbacks_.end() )
	{
	    if ( OnConnectHandle onConnectCallback = callback->lock() )
	    {
		(*onConnectCallback)(*this);
		++callback;
	    }
	    else
	    {
		// If a callback is no longer valid we remove it
	        boost::upgrade_lock<boost::shared_mutex> lock(callbacksMutex_);
		callback = onConnectCallbacks_.erase(callback);
	    }
	}
	CreateReceiver();
    }
    else if ( endpointIt != boost::asio::ip::tcp::resolver::iterator() )
    {
	boost::asio::ip::tcp::endpoint endpoint = *endpointIt;
	socket_.async_connect(endpoint,
			      boost::bind(&Connection::OnConnect, this,
					  boost::asio::placeholders::error,
					  ++endpointIt));
    }
    else
    {
	std::cerr<<"Failed to connect: "<<error.message()<<std::endl;
    }
}

void Connection::InitTimer()
{
    timer_.cancel();
    timer_.expires_from_now(
	boost::posix_time::seconds(CONNECTION_TIMEOUT_SECONDS));
    timer_.async_wait(boost::bind(&Connection::OnTimeOut, this, _1));
}

void Connection::OnTimeOut(const boost::system::error_code& error)
{
    if ( !error )
    {
	CheckConnection();
    }
}


void Connection::CreateReceiver()
{
    boost::lock_guard<boost::mutex> lock(socketMutex_);
    socket_.async_read_some(boost::asio::buffer(buffer_),
			    boost::bind(&Connection::Receive, this, _1, _2));
}

void Connection::Receive(const boost::system::error_code& error,
			 const std::size_t& bytes)
{
    if ( !error )
    {
	lastReception_ = time(0);
	InitTimer();

	std::vector<char> data(bytes);
	std::copy(buffer_.begin(), buffer_.begin()+bytes, data.begin());

	boost::shared_lock<boost::shared_mutex> lock(receiversMutex_);
	for(ReceiverContainer::iterator i = receivers_.begin();
	    i != receivers_.end();)
	{
	    if ( boost::shared_ptr<Receiver> receiver = i->lock() )
	    {
		(*receiver)(*this, data);
		++i;
	    }
	    else
	    {
		// If a receiver is no longer valid we remove it
		boost::upgrade_lock<boost::shared_mutex> lock(receiversMutex_);
		i = receivers_.erase(i);
	    }
	}
	CreateReceiver();
    }
    else
    {
	std::cerr<<"Connection::Receive: "<<error.message()<<std::endl;
	ioService_.stop();
    }
}

void Connection::Loop()
{
    std::cout<<"Connection thread "<<boost::this_thread::get_id()
	     <<" starting"<<std::endl;

    boost::unique_lock<boost::mutex> lock(workAvailableMutex_);

    while ( run_ )
    {
	ioService_.run();
	// If the service returns there's no work to do so check for timeouts
	CheckConnection();
	// Reset service flags so that it can be run again
	ioService_.reset();
	// Wait for work to do, use timeout to check for reconnects
	workAvailable_.timed_wait(lock,
				boost::posix_time::seconds(RECONNECT_TIMEOUT));
    }
    workAvailable_.notify_all();
    std::cout<<"Connection thread "<<boost::this_thread::get_id()
	     <<" ending"<<std::endl;
}

void Connection::CheckConnection()
{
    try
    {
	if ( IsConnected() && IsTimedOut() )
	{
	    std::clog<<"Connection to "<<host_<<":"<<port_<<" timed out after "
		     <<time(0)-lastReception_<<" seconds, reconnecting"
		     <<std::endl;
	    Reconnect();
	}
    }
    catch ( Exception& )
    {
    }
}
