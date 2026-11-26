#include "active_client_guard.hpp"

ActiveClientGuard::ActiveClientGuard(std::mutex &mutex, std::unordered_set<int> &clients, int fd)
    : active_clients(clients), queue_mutex(mutex), client_socket_fd(fd)
{
}

ActiveClientGuard::~ActiveClientGuard() noexcept
{
    std::lock_guard<std::mutex> lock(queue_mutex);
    active_clients.erase(client_socket_fd);
}