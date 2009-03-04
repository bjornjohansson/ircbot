#include "namedpipe.hpp"

NamedPipe::NamedPipe(const std::string& pipeName)
    : pipeFd_(open(pipeName.c_str(), O_RDWR))
    , pipe_(ioService_, *pipeFd_)
{
    if ( *pipeFd_ < 0 )
    {
	throw Exception(__FILE__, __LINE__, "Could not open named pipe");
    }
    CreateReceiver();
    thread_.reset(new boost::thread(boost::bind(&boost::asio::io_service::run,
						&ioService_)));
}

NamedPipe::ReceiverHandle NamedPipe::RegisterReceiver(Receiver receiver)
{
    ReceiverHandle handle(new Receiver(receiver));
    receivers_.push_back(ReceiverWeakPtr(handle));
    return handle;
}

void NamedPipe::OnReceive(const boost::system::error_code& error,
			  std::size_t bytes)
{
    if ( !error )
    {
	std::copy(buffer_.begin(), buffer_.begin()+bytes,
		  std::back_inserter(data_));

	std::string::size_type pos;
	if ( (pos=data_.find('\n')) != std::string::npos )
	{
	    std::string line = data_.substr(0, pos);
	    data_.erase(0, pos+1);
	    ReceiveLine(line);
	}
	CreateReceiver();
    }
    else
    {
	std::cerr<<"Named pipe failed: "<<error.message()<<std::endl;
    }
}

void NamedPipe::CreateReceiver()
{
    pipe_.async_read_some(boost::asio::buffer(buffer_),
			  boost::bind(&NamedPipe::OnReceive, this, _1, _2));
}

void NamedPipe::ReceiveLine(const std::string& line)
{
    for(ReceiverContainer::iterator i = receivers_.begin();
	i != receivers_.end();)
    {
	if ( ReceiverHandle receiver = i->lock() )
	{
	    std::stringstream ss(line);
	    std::string server, channel, message;
	    ss>>server>>channel;
	    while ( ss.peek() == ' ' ) { ss.get(); }
	    std::getline(ss, message);
	    try
	    {
		(*receiver)(message, channel, server);
	    }
	    catch ( Exception& e )
	    {
		std::cerr<<"Named pipe callback failed: "<<e.GetMessage()
			 <<std::endl;
	    }
	    ++i;
	}
	else
	{
	    // If a receiver is no longer valid we remove it
	    i = receivers_.erase(i);
	}
    }
}
