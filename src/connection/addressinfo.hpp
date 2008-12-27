#pragma once

#include <netdb.h>

#include <string>

class AddressInfo
{
public:
    /**
     * @throw Exception if an address can not be found for name
     */
    AddressInfo(const std::string& name, const std::string& service);
    virtual ~AddressInfo();

    const struct addrinfo* operator*() const { return address_; }
    const struct addrinfo* operator->() const { return address_; }

private:
    struct addrinfo* address_;
};
