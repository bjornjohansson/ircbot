#pragma once

#include <boost/thread/shared_mutex.hpp>

template<class T>
class thread_safe
{
public:
    thread_safe()
    {
    }

    thread_safe(T value)
    {
	AssignValue(value);
    }

    virtual ~thread_safe()
    {
	boost::lock_guard lock(mutex_);
    }

    thread_safe<T>& operator=(T value)
    {
	AssignValue(value);
    }

    T operator*() const
    {
	boost::shared_lock lock(mutex_);
	return value_;
    }
private:
    void AssignValue(T rhs)
    {
	boost::lock_guard lock(mutex_);
	value_ = rhs;
    }

    mutable boost::shared_mutex mutex_;
    
    T value_;
};
