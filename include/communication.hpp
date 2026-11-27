#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <cstddef>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include "config.hpp"
#include "logger.hpp"

int create_socket(Logger &logger, const std::string owner);
bool bind_socket(int socket_fd, const std::string &ip_address, int port, Logger &logger, const std::string owner);
bool start_listening(int socket_fd, int backlog, Logger &logger, const std::string owner);

// Client connection
int accept_client(int server_socket_fd,
                  sockaddr_in &client_address, Logger &logger, const std::string owner);

// Receive data from a connected client.
ssize_t receive_data(int socket_fd, char *buffer, std::size_t buffer_size);

// Send all the supplied data, handling partial sends.
bool send_all(int socket_fd, const char *data, std::size_t data_size, Logger &logger, const std::string owner);

#endif

// Handle client
void handle_client(int client_socket_fd, const ServerConfig &config, Logger &logger, const std::string owner);