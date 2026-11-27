#include "unique_socket.hpp"
#include <unistd.h>
#include <utility>
// Constructor
UniqueSocket::UniqueSocket(int fd) : client_socket_fd(fd)
{
}

/*
 This is also a valid implementation of the move constructor, but using std::exchange is more idiomatic in C++14 and later. 
It also makes the intention clearer that we are transferring ownership of the resource.

UniqueSocket::UniqueSocket(UniqueSocket &&other) noexcept
{
    client_socket_fd = other.client_socket_fd;
    other.client_socket_fd = -1;
}

*/

// Move constructor
UniqueSocket::UniqueSocket(UniqueSocket &&other) noexcept : client_socket_fd(std::exchange(other.client_socket_fd, -1))
{
}
// Move assignment operator
UniqueSocket &UniqueSocket::operator=(UniqueSocket &&other) noexcept
{
    if (this != &other)
    {
        reset(std::exchange(other.client_socket_fd, -1));
    }

    return *this;
}
// Destructor
UniqueSocket::~UniqueSocket()
{
    if (client_socket_fd >= 0)
    {
        close(client_socket_fd);
    }
}

int UniqueSocket::get() const noexcept
{
    return client_socket_fd;
}

// Release ownership without closing the descriptor
int UniqueSocket::release() noexcept
{
    return std::exchange(client_socket_fd, -1);
}

// Close the currently owned descriptor and adopt a new one
void UniqueSocket::reset(int new_fd) noexcept
{
    if (client_socket_fd >= 0)
    {
        ::close(client_socket_fd);
    }

    client_socket_fd = new_fd;
}