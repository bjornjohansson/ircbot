#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include <memory>
#include <list>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

class Connection;

class ConnectionManager
{
public:
    static ConnectionManager& Instance();

    /**
     * @throw Exception if unable to connect
     */
    boost::shared_ptr<Connection> Connect(const std::string& host,
					  unsigned short port);
    
    typedef boost::function<void ()> TimerCallback;

    /**
     * @param seconds The number of seconds to wait until timer fires
     */
    void RegisterTimer(time_t seconds, TimerCallback callback);

    void Loop();

    static void SignalHandler(int);

private:
    ConnectionManager();
    ConnectionManager(const ConnectionManager&);
    ConnectionManager& operator=(const ConnectionManager&);
    
    void CheckConnections();

    static std::auto_ptr<ConnectionManager> instance_;

//    std::auto_ptr<boost::thread> thread_;
    typedef boost::weak_ptr<Connection> ConnectionPtr;
    typedef std::list<ConnectionPtr> ConnectionContainer;
    ConnectionContainer connections_;

    // Unix timestamp when timer fires and callback
    typedef std::pair<time_t,TimerCallback> TimerCallbackPair;
    typedef std::list<TimerCallbackPair> TimerContainer;
    TimerContainer timers_;

    static int alarmPipe_[2];
    TimerCallback timerCallback_;
};



#endif
