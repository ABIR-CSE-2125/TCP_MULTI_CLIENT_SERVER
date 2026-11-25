#include "thread_pool.hpp"
#include "communication.hpp"
#include <stdexcept>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/socket.h>

void ::ThreadPool::worker_job()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queue_mutex); // Create a Lock over the client queue mutex

        condition.wait(lock, [this]
                       { return stopping || !client_queue.empty(); }); // Wait until there is a client to process or the pool is stopping

        if (stopping && client_queue.empty())
        {
            return; // Exit the thread if stopping and no clients are left
        }
        int client_socket_fd = client_queue.front();
        client_queue.pop();
        active_clients.insert(client_socket_fd);
        lock.unlock();                           // Unlock the mutex before processing the client
        handle_client(client_socket_fd, config); // Pass the config to handle_client
        std::cout << "Debug: Finished handling client: " << client_socket_fd << "\n";
        lock.lock();
        std::cout << "Debug: Removing client From ACtive Queue: " << client_socket_fd << "\n";
        active_clients.erase(client_socket_fd);
        std::cout << "Debug: Closing client From ACtive Queue: " << client_socket_fd << "\n";
        close(client_socket_fd);
        lock.unlock();
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

bool ThreadPool::enqueue_client(int client_socket_fd)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (stopping)
        {
            return false; // Indicate failure to enqueue
        }
        client_queue.push(client_socket_fd);
    }

    condition.notify_one();
    return true; // Indicate success
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stopping = true;
        // Close all Queued client sockets
        const std::string shutdown_message_q = "SERVER_SHUTDOWN QUEUED\n";
        while (!client_queue.empty())
        {
            int client_socket_fd = client_queue.front();
            client_queue.pop();
            send_all(client_socket_fd, shutdown_message_q.c_str(),
                     shutdown_message_q.size());
            close(client_socket_fd); // Close the client socket
        }
        // Notify all active clients before interrupting them
        const std::string shutdown_message_a = "SERVER_SHUTDOWN ACTIVE\n";
        for (int client_socket_fd : active_clients)
        {
            send_all(client_socket_fd, shutdown_message_a.c_str(),
                     shutdown_message_a.size());
            ::shutdown(client_socket_fd, SHUT_RDWR);
        }
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

ThreadPool::~ThreadPool()
{
    shutdown();
}