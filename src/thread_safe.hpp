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
	std::cout<<"thread_safe::operator="<<std::endl;
	boost::upgrade_lock<boost::shared_mutex> lock(mutex_);
	std::cout<<"thread_safe::operator=("<<rhs<<")"<<std::endl;
	value_ = rhs;
	return *this;
    }

    T operator*() const
    {
	std::cout<<"thread_safe::operator*"<<std::endl;
	boost::shared_lock<boost::shared_mutex> lock(mutex_);
	std::cout<<"thread_safe::operator* returns "<<value_<<std::endl;
	return value_;
    }

private:
    T value_;
    mutable boost::shared_mutex mutex_;
};
