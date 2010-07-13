#pragma once

#include <boost/asio.hpp>

// A test server that will wait for a connection, send a welcome
// message and then send back whatever it receives.

const std::string TEST_SERVER_WELCOME_MESSAGE = "Welcome";

class TestServer
{
public:
    TestServer(unsigned short port);
    virtual ~TestServer();

    void Run();

private:
    void StartAccept();
    void OnConnect(const boost::system::error_code& error);

    unsigned short port_;
    boost::asio::io_service ioService_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
};
