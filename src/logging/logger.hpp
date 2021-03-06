#pragma once
#include "loglevel.hpp"
#include "logmanager.hpp"

#include <sstream>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include <converter.hpp>

class Logger
{
public:
    class LoggerProxy
    {
    public:
	explicit LoggerProxy(LogSinkPtr sink)
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
	    explicit LoggerProxyImpl(LogSinkPtr sink)
		: sink_(sink)
	    {
	    }
	    
	    virtual ~LoggerProxyImpl()
	    {
		    (*sink_)<<buffer_.str();
	    }
	    
	    template<class T>
	    LoggerProxyImpl& operator<<(const T& rhs)
	    {
		    buffer_<<rhs;
		    return *this;
	    }

	    LoggerProxyImpl& operator<<(const UnicodeString& rhs)
	    {
		    buffer_<<AsUtf8(rhs);
		    return *this;
	    }
	private:
	    LogSinkPtr sink_;
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
