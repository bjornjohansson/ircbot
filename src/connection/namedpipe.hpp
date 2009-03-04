#include "../exception.hpp"

#include <list>

#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

class NamedPipe : boost::noncopyable
{
public:
    /**
     * @throw Exception if pipe can not be opened
     */
    explicit NamedPipe(const std::string& pipeName);

    typedef boost::function<void (const std::string&,
				  const std::string&,
				  const std::string&)> Receiver;
    typedef boost::shared_ptr<Receiver> ReceiverHandle;
    
    /**
     * Register a receiver for pipe events. The receiver is valid
     * as long as the returned handle is kept alive somewhere.
     */
    ReceiverHandle RegisterReceiver(Receiver);
private:
    void OnReceive(const boost::system::error_code& error, std::size_t bytes);
    void CreateReceiver();
    void ReceiveLine(const std::string& line);

    class FileDescriptor
    {
    public:
	FileDescriptor(int fd) : fd_(fd) {}
	~FileDescriptor() { if ( fd_ > 0 ) { close(fd_); } }

	const int& operator*() const { return fd_; }
    private:
	int fd_;
    };

    typedef boost::weak_ptr<Receiver> ReceiverWeakPtr;
    typedef std::list<ReceiverWeakPtr> ReceiverContainer;
    std::list<ReceiverWeakPtr> receivers_;
    FileDescriptor pipeFd_;
    boost::asio::io_service ioService_;
    boost::asio::posix::stream_descriptor pipe_;
    boost::array<char, 1024> buffer_;
    std::string data_;
    std::auto_ptr<boost::thread> thread_;
};
