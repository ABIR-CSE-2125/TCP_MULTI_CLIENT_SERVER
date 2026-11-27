#ifndef ACTIVE_CLIENT_GUARD_HPP
#define ACTIVE_CLIENT_GUARD_HPP

#include <mutex>
#include <unordered_set>

class ActiveClientGuard
{
private:
    std::unordered_set<int> &active_clients;
    std::mutex &queue_mutex;
    int client_socket_fd;

public:
    ActiveClientGuard(std::mutex &mutex,std::unordered_set<int> &clients, int fd);
    ~ActiveClientGuard() noexcept;
    // Disable copying and moving
    ActiveClientGuard(const ActiveClientGuard &other) = delete;
    ActiveClientGuard &operator=(const ActiveClientGuard &) = delete;
    ActiveClientGuard(ActiveClientGuard &&) = delete;
    ActiveClientGuard &operator=(ActiveClientGuard &&) = delete;
};

#endif // ACTIVE_CLIENT_GUARD_HPP