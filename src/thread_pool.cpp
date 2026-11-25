#include "thread_pool.hpp"
#include <stdexcept>
#include "communication.hpp"

void ::ThreadPool::worker_job()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        condition.wait(lock, [this]
            { return stopping || !client_queue.empty(); });
        if (stopping && client_queue.empty())
            {
                return; // Exit the thread if stopping and no clients are left
            }
        int client_socket_fd = client_queue.front();
        client_queue.pop(); 
        lock.unlock(); // Unlock the mutex before processing the client
        handle_client(client_socket_fd, config); // Pass the config to handle_client
    }
}

ThreadPool::ThreadPool(std::size_t num_threads, const ServerConfig &config_param) : config(config_param)
{
    if (num_threads < 1)
    {
        throw std::invalid_argument("Thread pool must have at least one worker");
    }

    for (std::size_t i = 0; i < num_threads; ++i)
    {
        workers.emplace_back(&ThreadPool::worker_job, this);
    }
}

void ThreadPool::enqueue_client(int client_socket_fd)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        client_queue.push(client_socket_fd);
    }

    condition.notify_one();
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stopping = true;
    }
    condition.notify_all();

    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}