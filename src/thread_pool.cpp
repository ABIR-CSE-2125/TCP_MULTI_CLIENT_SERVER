#include "thread_pool.hpp"
#include "communication.hpp"
#include "unique_socket.hpp"

#include <stdexcept>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <mutex>
#include "active_client_guard.hpp"

void ::ThreadPool::worker_job()
{
    while (true)
    {
        try
        {

            std::unique_lock<std::mutex> lock(queue_mutex); // Create a Lock over the client queue mutex
            bool client_registered = false;
            condition.wait(lock, [this]
                           { return stopping || !client_queue.empty(); }); // Wait until there is a client to process or the pool is stopping

            if (stopping && client_queue.empty())
            {
                return; // Exit the thread if stopping and no clients are left
            }
            UniqueSocket client_socket = std::move(client_queue.front());
            client_queue.pop();
            // Register the client socket in the active clients set
            try
            {
                auto result = active_clients.insert(client_socket.get());
                client_registered = result.second; // Mark that the client has been registered
            }
            catch (const std::exception &e)
            {
                std::cerr << "Active Client Insertion Failed: " << e.what() << '\n';
            }
            catch (...)
            {
                std::cerr << "Active Client Insertion Failed: Unknown error\n";
            }

            if (!client_registered)
            {
                continue; // Skip processing if the client was not registered
            }

            ActiveClientGuard active_client_guard(queue_mutex, active_clients, client_socket.get()); // RAII guard to remove the client from active clients on scope exit
            lock.unlock();                                                                           // Unlock the mutex before processing the client

            // Handle the client connection
            try
            {
                handle_client(client_socket.get(), config);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Client handling failed: " << e.what() << '\n';
            }
            catch (...)
            {
                std::cerr << "Client handling failed: Unknown error\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return;
        }
        catch (...)
        {
            std::cerr << "Unknown error occurred in worker thread\n";
            return;
        }
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
        try
        {
            workers.emplace_back(&ThreadPool::worker_job, this);
        }
        catch (...)
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
            throw;
        }
    }
}

bool ThreadPool::enqueue_client(UniqueSocket client_socket)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (stopping)
        {
            return false; // Indicate failure to enqueue
        }
        client_queue.push(std::move(client_socket));
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
        while (!client_queue.empty())
        {
            client_queue.pop(); // Clear the queue, which will close the sockets due to UniqueSocket's destructor
        }
        // Notify all active clients before interrupting them
        for (int client_socket_fd : active_clients)
        {
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