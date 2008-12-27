#include "socket.hpp"
#include "../exception.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

Socket::Socket(int domain, int type, int protocol)
    : socket_(-1)
{
    socket_ = socket(domain, type, protocol);
    if ( socket_ == -1 )
    {
	throw Exception(__FILE__, __LINE__, "Could not open socket");
    }
}

Socket::~Socket()
{
    if ( socket_ != -1 )
    {
	close(socket_);
    }
}
