#pragma once

#include <boost/thread.hpp>

template<class T>
class thread_safe
{
public:
    thread_safe() {}
    explicit thread_safe(const T& rhs) : value_(rhs) {}
    ~thread_safe() { boost::upgrade_lock<boost::shared_mutex> lock(mutex_); }
    thread_safe<T>& operator=(const T& rhs)
    {
	boost::upgrade_lock<boost::shared_mutex> lock(mutex_);
	value_ = rhs;
	return *this;
    }

    T operator*() const
    {
	boost::shared_lock<boost::shared_mutex> lock(mutex_);
	return value_;
    }

private:
    T value_;
    mutable boost::shared_mutex mutex_;
};
