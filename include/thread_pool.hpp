#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include "config.hpp"


class ThreadPool
{
    private:
        std::vector<std::thread> workers;
        std::queue<int> client_queue;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stopping = false;
        void worker_job();
        ServerConfig config; // Store the server configuration
    public:
        ThreadPool(std::size_t num_threads, const ServerConfig &config_param);
        ~ThreadPool();
        bool enqueue_client(int client_socket_fd);
        void shutdown();
};

#endif