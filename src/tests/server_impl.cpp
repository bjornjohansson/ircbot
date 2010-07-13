#include "server_impl.hpp"
#include "server_connection_impl.hpp"

using boost::asio::ip::tcp;

ServerImpl::ServerImpl(unsigned short port)
    : acceptor_(ioService_, tcp::endpoint(tcp::v4(), port))
    , socket_(ioService_)
{
    thread_.reset(new boost::thread(boost::bind(&ServerImpl::Run, this));
}

ServerImpl::~ServerImpl()
{
    boost::unique_lock<boost::mutex> lock(runMutex_);
    run_ = false;
    ioService.stop();
    loopEnded_.wait(lock);
}

void ServerImpl::StartAcceptingConnections()
{
    ServerConnectionImplPtr impl(new ServerConnectionImpl(ioService_));
    ConnectionPtr connection(new Connection(impl));

    acceptor_.async_accept(impl->GetSocket(),
			   boost::bind(&ServerImpl::Accept,
				       this,
				       connection,
				       _1));
}

void ServerImpl::Run()
{
    while (IsRunning())
    {
	ioService_.run();
	ioService_.reset();
    }
    loopEnded_.notify_all();
}

bool ServerImpl::IsRunning()
{
    boost::lock_guard<boost::mutex> lock(runMutex_);
    return run_;
}


void ServerImpl::Accept(ConnectionPtr connection,
			const boost::system::error_code& error)
{
    if (!error)
    {
	OnAccept.Fire(connection);
    }
}
