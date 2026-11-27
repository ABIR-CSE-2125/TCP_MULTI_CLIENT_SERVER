#include "communication.hpp"
#include "config.hpp"
#include "logger.hpp"
//====================================================
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <vector>
#include <cerrno>
#include <cstring>
#include <sstream>

int create_socket(Logger &logger, const std::string owner)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        int error_code = errno;
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("socket creation failed: ") + strerror(error_code));
        return -1;
    }

    // std::cout << "Socket created successfully!\n";

    logger.log(LogLevel::INFO, owner, "Listening socket created successfully");
    return socket_fd;
}

// BIND SOCKET TO AN IP ADDRESS AND PORT
bool bind_socket(int socket_fd, const std::string &ip_address, int port, Logger &logger, const std::string owner)
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
        // perror("IP address conversion failed");
        int error_code = errno;
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("IP address conversion failed") + strerror(error_code));
        return false;
    }

    if (ip_status == 0)
    {
        // std::cerr << "Invalid IP address format\n";
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("Invalid IP address format"));
        return false;
    }

    if (bind(socket_fd,
             reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) < 0)
    {
        // perror("Bind failed");
        int error_code = errno;
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("Bind failed") + strerror(error_code));
        return false;
    }

    std::ostringstream oss;
    oss << "Socket bound to " << ip_address << ":" << port << "\n";
    logger.log(LogLevel::INFO, "SERVER", oss.str());
    return true;
}

// START LISTENING FOR INCOMING CONNECTIONS
bool start_listening(int socket_fd, int backlog, Logger &logger, const std::string owner)
{
    if (listen(socket_fd, backlog) < 0)
    {
        // perror("Listen failed");
        int error_code = errno;
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("Listen failed") + strerror(error_code));
        return false;
    }

    // std::cout << "Server is listening...\n";
    logger.log(
        LogLevel::INFO,
        owner,
        std::string("Server is listening..."));
    return true;
}

// ACCEPT AN INCOMING CLIENT
int accept_client(int server_socket_fd, sockaddr_in &client_address, Logger &logger, const std::string owner)
{
    socklen_t client_address_len = sizeof(client_address);

    int client_socket_fd = accept(
        server_socket_fd,
        reinterpret_cast<sockaddr *>(&client_address),
        &client_address_len);

    if (client_socket_fd < 0)
    {
        if (errno == EINTR)
        {
            return -1;
        }
        // perror("Accept failed");
        int error_code = errno;
        logger.log(
            LogLevel::ERROR,
            owner,
            std::string("Accept failed") + strerror(error_code));
        return -1;
    }
    std::ostringstream oss;
    oss << "Accepted connection from "
        << inet_ntoa(client_address.sin_addr)
        << ":"
        << ntohs(client_address.sin_port)
        << "\n";
    logger.log(
        LogLevel::INFO,
        owner,
        oss.str());
    return client_socket_fd;
}

ssize_t receive_data(int socket_fd, char *buffer, std::size_t buffer_size)
{
    return recv(socket_fd, buffer, buffer_size, 0);
}

bool send_all(int socket_fd, const char *data, std::size_t data_size, Logger &logger, const std::string owner)
{
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

            // perror("Send failed");
            int error_code = errno;
            logger.log(
                LogLevel::ERROR,
                owner,
                std::string("Send failed") + strerror(error_code));
            return false;
        }
        else if (bytes_sent == 0)
        {
            // std::cerr << "Send made no progress\n";
            logger.log(
                LogLevel::ERROR,
                owner,
                std::string("Send made no progress"));
            return false;
        }
        total_bytes_sent += static_cast<std::size_t>(bytes_sent);
    }
    return true;
}

void handle_client(int client_socket_fd, const ServerConfig &config, Logger &logger, const std::string owner)
{
    std::vector<char> buffer(config.buffer_size);
    while (true)
    {

        // Receive data from the client
        ssize_t bytes_received = receive_data(
            client_socket_fd,
            buffer.data(),
            buffer.size());
        if (bytes_received < 0)
        {
            // perror("Receive failed");
            int error_code = errno;
            logger.log(
                LogLevel::ERROR,
                owner,
                std::string("Receive failed") + strerror(error_code));
            break;
        }
        else if (bytes_received == 0)
        {

            // std::cout << "Connection Closed\n";
            logger.log(LogLevel::INFO, owner, "Connection Closed");
            // std::cout << "DEBUG: handle_client returning for FD: "
            //   << client_socket_fd << std::endl;

            break;
        }
        std::ostringstream oss;
        oss << "Received: " << bytes_received << " bytes";
        logger.log(LogLevel::INFO, owner, oss.str());

        // std::cout.write(buffer.data(), bytes_received);
        // std::cout << '\n';
        // Send the received data back to the client (echo)
        if (!send_all(
                client_socket_fd,
                buffer.data(),
                static_cast<std::size_t>(bytes_received),
                logger, owner))
        {
            break;
        }

        // std::cout << "Echo sent successfully\n";
        logger.log(LogLevel::INFO, owner, "Echo sent successfully");
    }
}