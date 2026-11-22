#include "communication.hpp"
#include "config.hpp"
//=======================================================
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <sys/types.h>
#include <vector>

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

        std::cout << "Accepted a connection from "
                  << inet_ntoa(client_address.sin_addr) << ":"
                  << ntohs(client_address.sin_port) << "\n";

        while (true)
        {
            // READ BLOCK
            std::vector<char> buffer(config.buffer_size);
            ssize_t bytes_received = receive_data(
                client_socket_fd,
                buffer.data(),
                buffer.size());
            if (bytes_received < 0)
            {
                perror("Receive failed");
                break;
            }
            else if (bytes_received == 0)
            {
                std::cout << "Client disconnected\n";
                break;
            }
            std::cout << "Received: " << bytes_received << " bytes\n"
                      << "Content: ";
            std::cout.write(buffer.data(), bytes_received);
            std::cout << '\n';
            // SEND BLOCK
            if (!send_all(
                    client_socket_fd,
                    buffer.data(),
                    static_cast<std::size_t>(bytes_received)))
            {
                break;
            }

            std::cout << "Echo sent successfully\n";
        }
        close(client_socket_fd);
    }
    close(socket_fd);
    return 0;
}
