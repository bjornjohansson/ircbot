#include "connection.hpp"
#include "addressinfo.hpp"
#include "../exception.hpp"
#include "../error.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#ifdef DEBUG
#include <iostream>
#endif

#include <boost/weak_ptr.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

const time_t CONNECTION_TIMEOUT_SECONDS = 300;

Connection::Connection(const std::string& host, const unsigned short port)
    : host_(host),
      port_(port),
      lastReception_(time(0)),
      connected_(false)
{
    Connect();
}

void Connection::Reconnect()
{
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
    if ( socket_ && connected_ )
    {
	ssize_t sent = 0;
	ssize_t size = 0;
	
	while ( sent != static_cast<ssize_t>(data.size()) && size >= 0 )
	{
	    size = send(**socket_, &data[sent], data.size()-sent, 0);
	    sent += size;
	}
	if ( size == -1 )
	{
	    throw Exception(__FILE__, __LINE__, "Could not send data");
	}
    }
    else
    {
	throw Exception(__FILE__, __LINE__,
			"Attempting to send on invalid connection");
    }
}

void Connection::Receive()
{
    std::vector<char> buffer(1024);

    ssize_t b = recv(**socket_, &buffer[0], buffer.size(), MSG_DONTWAIT);

    if ( b > 0 )
    {
	lastReception_ = time(0);

	std::vector<char> data(b);
	for(ssize_t i = 0; i<b; ++i)
	{
	    data[i] = buffer[i];
	}

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
    }
}

Connection::OnConnectHandle
Connection::RegisterOnConnectCallback(OnConnectCallback c)
{
    OnConnectHandle handle(new OnConnectCallback(c));
    onConnectCallbacks_.push_back(OnConnectCallbackPtr(handle));
    return handle;
}


int Connection::GetSocket() const
{
    if ( socket_ && **socket_ != -1 )
    {
	return **socket_;
    }
    throw Exception(__FILE__, __LINE__, "No valid socket");
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
    std::stringstream ss;
    ss<<port_;
    try
    {
	AddressInfo addressInfo(host_, ss.str());

	for (const struct addrinfo* address = *addressInfo;
	     address;
	     address = address->ai_next )
	{
	    if ( address->ai_socktype != SOCK_STREAM )
	    {
		continue;
	    }
	    socket_.reset(new Socket(address->ai_family,
				     address->ai_socktype,
				     address->ai_protocol));
	    connected_ = false;
	    if (connect(**socket_,address->ai_addr,address->ai_addrlen) == 0)
	    {
		// Set last reception time to current time to indicate that
		// this connection has refreshed its timeout timer
		lastReception_ = time(0);
		connected_ = true;
		
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

		break;
	    }
	}
    }
    catch ( Exception& e )
    {
	std::clog<<e.GetMessage()<<std::endl;
    }

    if ( !connected_ )
    {
	std::clog<<"Connecting failed"<<std::endl;
    }

/*
    if ( !connected )
    {
	std::string error = GetErrorDescription(errno);
	throw Exception(__FILE__, __LINE__, error);
    }
*/
}
