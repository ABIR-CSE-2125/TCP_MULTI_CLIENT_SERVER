#include "communication.hpp"
#include "config.hpp"
#include "thread_pool.hpp"
#include "unique_socket.hpp"
//=======================================================
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <signal.h>
#include <utility>

volatile std::sig_atomic_t shutdown_requested = 0;

void handle_shutdown_signal(int)
{
    shutdown_requested = 1;
}

int main()
{

    ServerConfig config;

    if (!load_config("config/server.conf", config))
    {
        std::cerr << "Failed to load server configuration. Exiting.\n";
        return 1;
    }

    int socket_fd = create_socket();

    if (socket_fd < 0)
        return 1;

    // BIND SOCKET
    if (!bind_socket(socket_fd, config.server_ip, config.server_port))
    {
        close(socket_fd);
        return 1;
    }

    // START LISTENING
    if (!start_listening(socket_fd, config.listen_backlog))
    {
        close(socket_fd);
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    ThreadPool thread_pool(2, config);

    while (!shutdown_requested) //  For  keep Searching for connections
    {
        // Accepting a connection (optional, for demonstration purposes)
        sockaddr_in client_address{};

        int client_socket_fd = accept_client(
            socket_fd,
            client_address);
        // Shutdown requested check
        if (client_socket_fd < 0)
        {
            if (errno == EINTR)
            {
                if (shutdown_requested)
                    break;

                continue;
            }
            std::cerr << "Failed to accept client connection. Continuing...\n";
            continue; // Continue to the next iteration to accept another connection
        }

        UniqueSocket client_socket(client_socket_fd);
        if (!thread_pool.enqueue_client(std::move(client_socket)))
        {
            std::cerr << "Failed to enqueue client. Continuing...\n";
        }
    }
    thread_pool.shutdown();
    close(socket_fd);
    return 0;
}
