#ifndef CONNECTIONMANAGER_HPP
#define CONNECTIONMANAGER_HPP

#include <memory>
#include <list>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

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

private:
    ConnectionManager();
    ConnectionManager(const ConnectionManager&);
    ConnectionManager& operator=(const ConnectionManager&);

    void Loop();
    void CheckConnections();

    static std::auto_ptr<ConnectionManager> instance_;

    std::auto_ptr<boost::thread> thread_;

    boost::asio::io_service ioService_;

    typedef boost::weak_ptr<Connection> ConnectionPtr;
    typedef std::list<ConnectionPtr> ConnectionContainer;
    ConnectionContainer connections_;
    boost::mutex loopMutex_;
    boost::condition_variable loopCondition_;
};

#endif
