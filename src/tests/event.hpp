#pragma once

#include <boost/shared_ptr.hpp>

class NoType
{
};

template<typename ParamType1 = NoType,
	 typename ParamType2 = NoType,
	 typename ParamType3 = NoType,
	 typename ParamType4 = NoType,
	 typename ParamType5 = NoType,
	 typename ParamType6 = NoType,
	 typename ParamType7 = NoType,
	 typename ParamType8 = NoType,
	 typename ParamType9 = NoType,
	 typename ParamType10= NoType>
class Event
{
public:
    typedef boost::shared_ptr<HandlerFunction> Handler;
private:
    typedef std::list<boost::weak_ptr<HandlerFunction> > HandlerContainer;

    class Iterator
    {
    public:
	Iterator(HandlerContainer& container)
	    : container_(container)
	{
	}
	
	Handler Next()
	{
	    if (iterator_ == container_.end())
	    {
		return Handler();
	    }
	    if (Handler handler = iterator_->lock())
	    {
		++iterator_;
		return handler;
	    }
	    iterator_ = container_.erase(iterator_);
	    return Next();
	}

    private:
	ContainerType& container_;
	ContainerType::iterator iterator_;
    };
public:
    typedef boost::shared_ptr<HandlerFunction> Handler;

    Event();

    Handler AddHandler(HandlerFunction function)
    {
	boost::lock_guard lock(handlersMutex_);
	handlers_.push_back(function);
    }

    int Fire()
    {
	boost::lock_guard lock(handlersMutex_);
	Iterator iterator(handlers_);
	while (Handler handler = iterator.Next())
	{
	    (*handler)();
	}
    }

    int Fire(ParamType1 param1)
    {
	boost::lock_guard lock(handlersMutex_);
	Iterator iterator(handlers_);
	while (Handler handler = iterator.Next())
	{
	    (*handler)(param1);
	}
    }


private:
    HandlerContainer handlers_;

    boost::mutex handlersMutex_;

};
