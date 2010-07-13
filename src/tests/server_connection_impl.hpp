#pragma once

class ServerConnectionImpl
{
public:
    ServerConnectionImpl(boost::asio::io_service& ioService)
	: socket_(ioService)
    {
    }

    boost::asio::ip::tcp::socket& GetSocket()
    {
	return socket_;
    }

private:
    boost::asio::ip::tcp::socket socket_;

};

typedef boost::shared_ptr<ServerConnectionImpl> ServerConnectionImplPtr;
