#include "communication.hpp"
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>

int create_socket()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    std::cout << "Socket created successfully!\n";
    return socket_fd;
}

// BIND SOCKET TO AN IP ADDRESS AND PORT
bool bind_socket(int socket_fd,
                 const std::string &ip_address,
                 int port)
{
    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    int ip_status = inet_pton(
        AF_INET,
        ip_address.c_str(),
        &server_address.sin_addr);

    if (ip_status < 0)
    {
        perror("IP address conversion failed");
        return false;
    }

    if (ip_status == 0)
    {
        std::cerr << "Invalid IP address format\n";
        return false;
    }

    if (bind(socket_fd,
             reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        return false;
    }

    std::cout << "Socket bound to "
              << ip_address << ":" << port << "\n";

    return true;
}

// START LISTENING FOR INCOMING CONNECTIONS
bool start_listening(int socket_fd, int backlog)
{
    if (listen(socket_fd, backlog) < 0)
    {
        perror("Listen failed");
        return false;
    }

    std::cout << "Server is listening...\n";
    return true;
}

// ACCEPT AN INCOMING CLIENT
int accept_client(int server_socket_fd,
                  sockaddr_in &client_address)
{
    socklen_t client_address_len = sizeof(client_address);

    int client_socket_fd = accept(
        server_socket_fd,
        reinterpret_cast<sockaddr *>(&client_address),
        &client_address_len);

    if (client_socket_fd < 0)
    {
        perror("Accept failed");
        return -1;
    }

    std::cout << "Accepted connection from "
              << inet_ntoa(client_address.sin_addr)
              << ":"
              << ntohs(client_address.sin_port)
              << "\n";

    return client_socket_fd;
}

ssize_t receive_data(int socket_fd, char *buffer, std::size_t buffer_size)
{
    return recv(socket_fd, buffer, buffer_size, 0);
}

bool send_all(int socket_fd, const char *data, std::size_t data_size){
    
    std::size_t total_bytes_sent = 0;
    while (total_bytes_sent < data_size)
    {
        ssize_t bytes_sent = send(socket_fd,
                                  data + total_bytes_sent,
                                  data_size - total_bytes_sent,
                                  MSG_NOSIGNAL);
        if (bytes_sent < 0)
        {
            if (errno == EINTR)
                continue;

            perror("Send failed");
            return false;
        }
        else if (bytes_sent == 0)
        {
            std::cerr << "Send made no progress\n";
            return false;
        }
        total_bytes_sent += static_cast<std::size_t>(bytes_sent);
    }
    return true;
}