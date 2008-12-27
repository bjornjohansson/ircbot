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
int ConnectionManager::alarmPipe_[2] = {-1,-1};

// Number of seconds before select times out
const int SELECT_TIMEOUT = 30;

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
  boost::shared_ptr<Connection> connection(new Connection(h, port));

  connections_.push_back(boost::weak_ptr<Connection>(connection));

  return connection;
}


void ConnectionManager::RegisterTimer(time_t seconds,
				      TimerCallback callback)
{
    if ( seconds >= 0 )
    {
	std::cerr<<"Registering timer in "<<seconds<<" seconds"<<std::endl;

	struct sigaction action;
	action.sa_handler = &ConnectionManager::SignalHandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	
	if ( sigaction(SIGALRM, &action, 0) == -1 )
	{
	    std::cerr<<"Could not install signal handler: "
		     <<GetErrorDescription(errno)<<std::endl;
	}
	alarm(static_cast<unsigned int>(seconds));
    }
    timerCallback_ = callback;
}

ConnectionManager::ConnectionManager()
{
    // Disable threading for now, let the client call Loop and handle stuff through callbacks instead
//  thread_.reset(new boost::thread(boost::bind(&ConnectionManager::Loop,this)));

    alarmPipe_[0] = -1;
    alarmPipe_[1] = -1;
    if ( pipe(alarmPipe_) == -1 )
    {
	std::cerr<<"Could not open alarm pipe: "
		 <<GetErrorDescription(errno)<<std::endl;
    }
    else
    {
	// Set both ends of the pipe to be non-blocking
	int flags = fcntl(alarmPipe_[0], F_GETFL);
	if ( flags != -1 )
	{
	    fcntl(alarmPipe_[0], F_SETFL, flags | O_NONBLOCK);
	}
	flags = fcntl(alarmPipe_[1], F_GETFL);
	if ( flags != -1 )
	{
	    fcntl(alarmPipe_[1], F_SETFL, flags | O_NONBLOCK);
	}
    }
}

void ConnectionManager::Loop()
{
  bool run = true;

  while(run)
  {
      fd_set readDescriptors;
      FD_ZERO(&readDescriptors);

      int maxDescriptor = 0;

      typedef std::map<int,ConnectionPtr> SelectedMap;
      SelectedMap selectedConnections;

      for(ConnectionContainer::iterator i = connections_.begin();
	  i != connections_.end();
	  ++i)
      {
	  if ( boost::shared_ptr<Connection> connection = i->lock() )
	  {
	      if ( connection->IsConnected() )
	      {
		  try
		  {
		      int s = connection->GetSocket();
		      maxDescriptor = std::max(s, maxDescriptor);
		      FD_SET(s, &readDescriptors);
		      selectedConnections[s] = *i;
		  }
		  catch ( Exception& )
		  {
		  }
	      }
	  }
      }

      // Select read-end of our alarm pipe
      if ( alarmPipe_[0] != -1 )
      {
	  maxDescriptor = std::max(alarmPipe_[0], maxDescriptor);
	  FD_SET(alarmPipe_[0], &readDescriptors);
      }

      if ( maxDescriptor > 0 )
      {
	  struct timeval timeout;
	  timeout.tv_sec = SELECT_TIMEOUT;
	  timeout.tv_usec = 0;
	  int res = select(maxDescriptor+1, &readDescriptors, 0, 0, &timeout);
	  if ( res >= 0 )
	  {
	      // Data available, check if it's one of our connections
	      for(SelectedMap::iterator i = selectedConnections.begin();
		  i != selectedConnections.end();
		  ++i)
	      {
		  if ( FD_ISSET(i->first, &readDescriptors) )
		  {
		      if (boost::shared_ptr<Connection> con = i->second.lock())
		      {
			  con->Receive();
		      }
		  }
	      }
	      // Check if it's our alarm pipe
	      if ( alarmPipe_[0] != -1 &&
		   FD_ISSET(alarmPipe_[0], &readDescriptors) )
	      {
		  // Clear the alarm pipe
		  char buffer[1];
		  while ( read(alarmPipe_[0], &buffer, 1) > 0 )
		  {
		      ;
		  }
		  // And use the callback for the alarm timer
		  if ( timerCallback_ )
		  {
		      timerCallback_();
		  }
	      }
	  }
	  else if ( res == 0 )
	  {
	      // Timeout
	  }
	  else if ( res == -1 )
	  {
	      std::cerr<<"select failed: ("<<errno<<") "
		       <<GetErrorDescription(errno)<<std::endl;
	  }
	  CheckConnections();
      }
      else
      {
	  sleep(SELECT_TIMEOUT);
      }
  }
}

void ConnectionManager::SignalHandler(int)
{
    if ( alarmPipe_[1] != -1 )
    {
	char buffer[] = { '\0' };
	write(alarmPipe_[1], buffer, 1);
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
