#pragma once
#include "loglevel.hpp"
#include "logmanager.hpp"

#include <sstream>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

class Logger
{
public:
    class LoggerProxy
    {
    public:
	explicit LoggerProxy(LogSink& sink)
	    : impl_(new LoggerProxyImpl(sink))
	{
	}

	template<class T>
	LoggerProxy& operator<<(const T& rhs)
	{
	    (*impl_)<<rhs;
	    return *this;
	}
    private:
	class LoggerProxyImpl : boost::noncopyable
	{
	public:
	    explicit LoggerProxyImpl(LogSink& sink)
		: sink_(sink)
	    {
	    }
	    
	    virtual ~LoggerProxyImpl()
	    {
		sink_<<buffer_.str();
	    }
	    
	    template<class T>
	    LoggerProxyImpl& operator<<(const T& rhs)
	    {
		buffer_<<rhs;
		return *this;
	    }
	private:
	    LogSink& sink_;
	    std::stringstream buffer_;
	};

	boost::shared_ptr<LoggerProxyImpl> impl_;
    };

    Logger();

    LoggerProxy operator<<(const LogLevel::LogLevel logLevel)
    {
	LoggerProxy proxy(LogManager::Instance().GetSink(logLevel));
	return proxy;
    }
};

extern Logger Log;
