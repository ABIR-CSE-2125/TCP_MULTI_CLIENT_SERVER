#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <unordered_set>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include "config.hpp"
#include "unique_socket.hpp"

class ThreadPool
{
    private:
        std::vector<std::thread> workers;
        std::queue<UniqueSocket> client_queue;
        std::unordered_set<int> active_clients;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stopping = false;
        void worker_job();
        ServerConfig config; // Store the server configuration
    public:
        ThreadPool(std::size_t num_threads, const ServerConfig &config_param);
        ~ThreadPool();
        bool enqueue_client(UniqueSocket client_socket_fd);
        void shutdown();
};

#endif