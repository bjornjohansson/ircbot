#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "event.hpp"
#include "connection.fwd.hpp"

class ServerImpl
{
public:
    ServerImpl(unsigned short port);
    virtual ~ServerImpl();

    void StartAcceptingConnections();

    Event<ConnectionPtr> OnAccept;

private:
    void Run();
    bool IsRunning();

    unsigned short port_;
    bool run_;

    boost::asio::io_service ioService_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;

    boost::condition_variable loopEnded_;
    boost::mutex runMutex_;

    boost::shared_ptr<boost::thread> thread_;
};
