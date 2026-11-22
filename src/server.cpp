#include "communication.hpp"
#include "config.hpp"
//=======================================================
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
//=======================================================
#include <thread>
#include <functional>
#include <system_error>
#include <exception>

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

    while (true) //  For  keep Searching for connections
    {
        // Accepting a connection (optional, for demonstration purposes)
        sockaddr_in client_address{};

        int client_socket_fd = accept_client(
            socket_fd,
            client_address);

            if (client_socket_fd < 0){
                
                std::cerr << "Failed to accept client connection. Continuing...\n";
                continue; // Continue to the next iteration to accept another connection
            }

        // Handle the client in a separate thread
            try
            {
                std::thread client_thread(
                    handle_client,
                    client_socket_fd,
                    std::cref(config));

                try
                {
                    client_thread.detach();
                }
                catch (const std::system_error &e)
                {
                    std::cerr << "Thread detach failed: "
                              << e.what() << '\n';

                    if (client_thread.joinable())
                    {
                        client_thread.join();
                    }
                }
            }
            catch (const std::system_error &e)
            {
                std::cerr << "Thread creation failed: "
                          << e.what() << '\n';
                close(client_socket_fd);
            }
    }
    close(socket_fd);
    return 0;
}
