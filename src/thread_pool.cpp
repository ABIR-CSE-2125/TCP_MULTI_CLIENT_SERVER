#include "thread_pool.hpp"
#include "communication.hpp"
#include "unique_socket.hpp"
#include "logger.hpp"

#include <stdexcept>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <mutex>
#include "active_client_guard.hpp"

constexpr const char *OWNER = "THREAD_POOL";

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
                // std::cerr << "Active Client Insertion Failed: " << e.what() << '\n';
                logger.log(LogLevel::ERROR, OWNER, std::string("Active Client Insertion Failed: ") + e.what());
            }
            catch (...)
            {
                // std::cerr << "Active Client Insertion Failed: Unknown error\n";
                logger.log(LogLevel::ERROR, OWNER, "Active Client Insertion Failed: Unknown error");
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
                handle_client(client_socket.get(), config, logger, OWNER);
            }
            catch (const std::exception &e)
            {
                // std::cerr << "Client handling failed: " << e.what() << '\n';
                logger.log(LogLevel::ERROR, OWNER, std::string("Client handling failed: ") + e.what());
            }
            catch (...)
            {
                // std::cerr << "Client handling failed: Unknown error\n";
                logger.log(LogLevel::ERROR, OWNER, "Client handling failed: Unknown error");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            return;
        }
        catch (...)
        {
            // std::cerr << "Unknown error occurred in worker thread\n";
            logger.log(LogLevel::ERROR, OWNER, "Unknown error occurred in worker thread");
            return;
        }
    }
}

ThreadPool::ThreadPool(std::size_t num_threads, const ServerConfig &config_param, Logger &logger_param) : config(config_param), logger(logger_param)
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
        logger.log(LogLevel::INFO, OWNER,
                   "Shutdown initiated");
        // Close all Queued client sockets
        logger.log(LogLevel::INFO, OWNER, "Cleaning Up " + std::to_string(client_queue.size()) + " Pending Clients");
        while (!client_queue.empty())
        {
            client_queue.pop(); // Clear the queue, which will close the sockets due to UniqueSocket's destructor
        }
        logger.log(LogLevel::INFO, OWNER,"Pending Client Cleanup Completed");
        // Notify all active clients before interrupting them
        logger.log(LogLevel::INFO, OWNER, "Cleaning Up " + std::to_string(active_clients.size()) + " Active Client  Cleanup Start");
        for (int client_socket_fd : active_clients)
        {
            ::shutdown(client_socket_fd, SHUT_RDWR);
            logger.log(LogLevel::INFO, OWNER,
                       "Active client shutdown signals sent");
        }
    }
    condition.notify_all();
    logger.log(LogLevel::INFO, OWNER,
               "Waiting for worker threads to terminate");

    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    logger.log(LogLevel::INFO, OWNER,
               "All worker threads terminated. Shutdown successful");
}

ThreadPool::~ThreadPool()
{
    shutdown();
}