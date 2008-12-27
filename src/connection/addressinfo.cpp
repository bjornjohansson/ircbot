#include "addressinfo.hpp"
#include "../exception.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <cassert>

AddressInfo::AddressInfo(const std::string& name, const std::string& service)
    : address_(0)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ( getaddrinfo(name.c_str(), service.c_str(), &hints, &address_) != 0 )
    {
	throw Exception(__FILE__, __LINE__,
			std::string("Could not get address for ")+name);
    }
    assert(address_);
}

AddressInfo::~AddressInfo()
{
    if ( address_ )
    {
	freeaddrinfo(address_);
    }
}
