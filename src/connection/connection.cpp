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

const time_t CONNECTION_TIMEOUT_SECONDS = 300;

Connection::Connection(boost::asio::io_service& ioService,
		       const std::string& host,
		       const unsigned short port)
    : ioService_(ioService),
      socket_(ioService),
      host_(host),
      port_(port),
      lastReception_(time(0)),
      connected_(false)
{
    Connect();
}

void Connection::Reconnect()
{
    socket_.close();
    Connect();
}

Connection::ReceiverHandle Connection::RegisterReceiver(Receiver r)
{
    ReceiverHandle handle(new Receiver(r));
    receivers_.push_back(boost::weak_ptr<Receiver>(handle));
    return handle;
}

void Connection::Send(const std::vector<char>& data)
{
    if ( socket_.is_open() && connected_ )
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
    onConnectCallbacks_.push_back(OnConnectCallbackPtr(handle));
    return handle;
}


bool Connection::IsTimedOut() const
{
    return lastReception_+CONNECTION_TIMEOUT_SECONDS < time(0);
}

bool Connection::IsConnected() const
{
    return connected_;
}

void Connection::Connect()
{
    try
    {
	std::clog<<"Connecting to "<<host_<<":"<<port_<<"..."<<std::endl;
	// Create a resolver and a query for the supplied host and port
	using namespace boost::asio::ip;
	tcp::resolver resolver(ioService_);
	tcp::resolver::query query(host_,
				   boost::lexical_cast<std::string>(port_));

	tcp::resolver::iterator endpointIt = resolver.resolve(query);
	tcp::endpoint endpoint = *endpointIt;
	socket_.async_connect(endpoint,
			      boost::bind(&Connection::OnConnect, this,
					  boost::asio::placeholders::error,
					  ++endpointIt));
    }
    catch ( boost::bad_lexical_cast& e )
    {
	throw Exception(__FILE__, __LINE__, e.what());
    }
}

void Connection::OnConnect(const boost::system::error_code& error,
			   boost::asio::ip::tcp::resolver::iterator endpointIt)
{
    if ( !error )
    {
	connected_ = true;

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


void Connection::CreateReceiver()
{
    socket_.async_read_some(boost::asio::buffer(buffer_),
			    boost::bind(&Connection::Receive, this, 
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void Connection::Receive(const boost::system::error_code& error,
			 const std::size_t& bytes)
{
    if ( !error )
    {
	lastReception_ = time(0);

	std::vector<char> data(bytes);
	std::copy(buffer_.begin(), buffer_.begin()+bytes, data.begin());

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
		i = receivers_.erase(i);
	    }
	}
	CreateReceiver();
    }
    else
    {
	std::cerr<<error.message()<<std::endl;
    }
}
