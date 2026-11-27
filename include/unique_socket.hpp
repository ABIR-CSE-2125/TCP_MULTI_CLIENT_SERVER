
#ifndef UNIQUE_SOCKET_HPP
#define UNIQUE_SOCKET_HPP

class UniqueSocket
{
private:
    int client_socket_fd;

public:
    // Construction and destruction
    explicit UniqueSocket(int fd = -1);
    ~UniqueSocket() noexcept;

    // Disable copying
    UniqueSocket(const UniqueSocket &other) = delete;
    UniqueSocket &operator=(const UniqueSocket &) = delete;

    // Enable moving
    UniqueSocket(UniqueSocket &&other) noexcept;
    UniqueSocket &operator=(UniqueSocket &&other) noexcept;

    // Resource access and management
    int get() const noexcept;
    int release() noexcept;
    void reset(int new_fd = -1) noexcept;
};

#endif
