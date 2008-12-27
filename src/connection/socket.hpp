#pragma once

class Socket
{
public:
    /**
     * @throw Exception if socket cannot be opened
     */
    Socket(int domain, int type, int protocol);
    virtual ~Socket();

    int operator*() const { return socket_; }

private:
    int socket_;
};
