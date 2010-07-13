#include "testserver.hpp"

#include <boost/bind.hpp>

using boost::asio::ip::tcp;

TestServer::TestServer(unsigned short port)
    : port_(port)
    , acceptor_(ioService_, tcp::endpoint(tcp::v4(), port_))
    , socket_(ioService_)
{
    StartAccept();
}

TestServer::~TestServer()
{
    ioService_.stop();
}

void TestServer::StartAccept()
{
    acceptor_.async_accept(socket_,
			   boost::bind(&TestServer::OnConnect, this, _1));
}

void TestServer::OnConnect(const boost::system::error_code& error)
{
    if (!error)
    {
	boost::system::error_code writeError;
	boost::asio::write(socket_,
			   boost::asio::buffer(TEST_SERVER_WELCOME_MESSAGE),
			   boost::asio::transfer_all(),
			   writeError);
	StartAccept();
    }
}

void TestServer::Run()
{
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port_);
    boost::asio::ip::tcp::acceptor acceptor(ioService_, endpoint);

    while(true)
    {
	boost::asio::ip::tcp::socket socket(ioService_);
	acceptor.accept(socket);

	boost::system::error_code error;
	boost::asio::write(socket,
			   boost::asio::buffer(TEST_SERVER_WELCOME_MESSAGE),
			   boost::asio::transfer_all(),
			   error);
    }

}
