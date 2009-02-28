#include "connectionmanager.hpp"
#include "connection.hpp"
#include "../exception.hpp"
#include "../error.hpp"

#include <boost/bind.hpp>
#include <map>
#include <iostream>

#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

std::auto_ptr<ConnectionManager> ConnectionManager::instance_;

// Number of seconds before attempting a new reconnect
const int RECONNECT_TIMEOUT = 30;

ConnectionManager& ConnectionManager::Instance()
{
    if ( instance_.get() == 0 )
    {
	instance_.reset(new ConnectionManager);
    }
    return *instance_;
}

boost::shared_ptr<Connection> ConnectionManager::Connect(const std::string& h,
							 unsigned short port)
{
    boost::shared_ptr<Connection> connection(new Connection(ioService_,
							    h,
							    port));

    connections_.push_back(boost::weak_ptr<Connection>(connection));

    loopCondition_.notify_all();

    return connection;
}

ConnectionManager::ConnectionManager()
    : run_(true)
{
    thread_.reset(new boost::thread(boost::bind(&ConnectionManager::Loop,
						this)));
}

ConnectionManager::~ConnectionManager()
{
    boost::unique_lock<boost::mutex> lock(loopMutex_);
    run_ = false;
    loopCondition_.notify_all();
}

void ConnectionManager::Loop()
{
    std::cout<<"ConnectionManager thread starting"<<std::endl;

    while ( run_ )
    {
	// Wait for a set amount of time between each run to throttle
	// reconnect attempts. Any newly created connections will signal
	// the condition to immediately run the service.
	boost::unique_lock<boost::mutex> lock(loopMutex_);
	loopCondition_.timed_wait(lock,
				  boost::posix_time::seconds(RECONNECT_TIMEOUT));
	ioService_.run();
	CheckConnections();
	// The service needs to be reset before another run() call to clear
	// any flags (e.g. stopped) that might prevent it to pick up on
	// new connections or whatever needs to be picked up.
	ioService_.reset();
    }
}

void ConnectionManager::CheckConnections()
{
    for(ConnectionContainer::iterator i = connections_.begin();
	i != connections_.end();
	++i)
    {
        if ( boost::shared_ptr<Connection> connection = i->lock() )
	{
	  try
	  {
	      if ( connection->IsTimedOut() || !connection->IsConnected() )
	      {
		  std::clog<<"Attempting to reconnect"<<std::endl;
		  connection->Reconnect();
	      }
      	  }
	  catch ( Exception& )
	  {
	  }
	}
    }
}
