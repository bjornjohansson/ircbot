#pragma once

#include "connection.fwd.hpp"

class ConnectionImpl
{
public:
    virtual void Send(const std::vector<char>& data) = 0;
};

class Connection
{
public:
    typedef boost::shared_ptr<ConnectionImpl> ImplPtr;

    Connection(ImplPtr impl)
	: impl_(impl)
    {
    }

    void Send(const std::vector<char>& data)
    {
	impl_.Send(data);
    }
private:
    ImplPtr impl_;
};
