
#include "thread_pool.hpp"
#include "communication.hpp"
#include "unique_socket.hpp"
#include "logger.hpp"
#include "active_client_guard.hpp"
//===========================================================
#include <stdexcept>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <mutex>

constexpr const char *OWNER = "THREAD_POOL";

void ThreadPool::worker_job()
{
    while (true)
    {
        try
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            bool client_registered = false;

            condition.wait(lock, [this]
                           { return stopping || !client_queue.empty(); });

            if (stopping && client_queue.empty())
            {
                return;
            }

            UniqueSocket client_socket = std::move(client_queue.front());
            client_queue.pop();

            int client_fd = client_socket.get();

            // Register client in active client set
            try
            {
                auto result = active_clients.insert(client_fd);
                client_registered = result.second;
            }
            catch (const std::exception &e)
            {
                logger.log(
                    LogLevel::WARN, OWNER,
                    std::string("Active client registration failed: ") + e.what());
            }
            catch (...)
            {
                logger.log(
                    LogLevel::WARN, OWNER,
                    "Active client registration failed: Unknown error");
            }

            if (!client_registered)
            {
                logger.log(
                    LogLevel::WARN, OWNER,
                    "Client registration failed. Skipping client (FD: " +
                        std::to_string(client_fd) + ")");

                continue;
            }

            ActiveClientGuard active_client_guard(
                queue_mutex, active_clients, client_fd);

            lock.unlock();

            logger.log(
                LogLevel::DEBUG, OWNER,
                "Client assigned to worker (FD: " +
                    std::to_string(client_fd) + ")");

            // Handle client connection
            try
            {
                handle_client(client_fd, config, logger, OWNER);
            }
            catch (const std::exception &e)
            {
                logger.log(
                    LogLevel::WARN, OWNER,
                    std::string("Client handling failed (FD: ") +
                        std::to_string(client_fd) + "): " + e.what());
            }
            catch (...)
            {
                logger.log(
                    LogLevel::WARN, OWNER,
                    "Client handling failed with unknown error (FD: " +
                        std::to_string(client_fd) + ")");
            }

            logger.log(
                LogLevel::DEBUG, OWNER,
                "Client processing finished (FD: " +
                    std::to_string(client_fd) + ")");
        }
        catch (const std::exception &e)
        {
            logger.log(
                LogLevel::ERROR, OWNER,
                std::string("Unexpected worker thread error: ") + e.what());
            return;
        }
        catch (...)
        {
            logger.log(
                LogLevel::ERROR, OWNER,
                "Unknown error occurred in worker thread");
            return;
        }
    }
}

ThreadPool::ThreadPool(
    std::size_t num_threads,
    const ServerConfig &config_param,
    Logger &logger_param)
    : config(config_param), logger(logger_param)
{
    if (num_threads < 1)
    {
        throw std::invalid_argument(
            "Thread pool must have at least one worker");
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
    int client_fd = client_socket.get();

    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        if (stopping)
        {
            logger.log(
                LogLevel::WARN, OWNER,
                "Client rejected because thread pool is stopping (FD: " +
                    std::to_string(client_fd) + ")");

            return false;
        }

        client_queue.push(std::move(client_socket));
    }

    logger.log(
        LogLevel::DEBUG, OWNER,
        "Client added to pending queue (FD: " +
            std::to_string(client_fd) + ")");

    condition.notify_one();

    return true;
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        stopping = true;

        logger.log(
            LogLevel::INFO, OWNER,
            "===================== Shutdown Initiated =====================");

        logger.log(
            LogLevel::INFO, OWNER,
            "Cleaning up " + std::to_string(client_queue.size()) +
                " pending client(s)");

        while (!client_queue.empty())
        {
            client_queue.pop();
        }

        logger.log(
            LogLevel::INFO, OWNER,
            "Pending client cleanup completed");

        logger.log(
            LogLevel::INFO, OWNER,
            "Cleaning up " + std::to_string(active_clients.size()) +
                " active client(s)");

        int successful_shutdowns = 0;
        int failed_shutdowns = 0;

        for (int client_socket_fd : active_clients)
        {
            if (::shutdown(client_socket_fd, SHUT_RDWR) == 0)
            {
                ++successful_shutdowns;
            }
            else
            {
                int error_code = errno;
                ++failed_shutdowns;

                logger.log(
                    LogLevel::WARN, OWNER,
                    "Failed to signal shutdown for client FD " +
                        std::to_string(client_socket_fd) + ": " +
                        strerror(error_code));
            }
        }

        logger.log(
            LogLevel::INFO, OWNER,
            "Active client shutdown signals: " +
                std::to_string(successful_shutdowns) + " successful, " +
                std::to_string(failed_shutdowns) + " failed");
    }

    condition.notify_all();

    logger.log(
        LogLevel::INFO, OWNER,
        "Waiting for worker threads to terminate");

    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    logger.log(
        LogLevel::INFO, OWNER,
        "All worker threads terminated. Shutdown successful");
}

ThreadPool::~ThreadPool()
{
    shutdown();
}
