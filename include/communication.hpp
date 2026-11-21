#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <cstddef>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

int create_socket();
bool bind_socket(int socket_fd, const std::string &ip_address, int port);
bool start_listening(int socket_fd, int backlog);

// Client connection
int accept_client(int server_socket_fd,
                  sockaddr_in &client_address);

// Receive data from a connected client.
ssize_t receive_data(int socket_fd, char *buffer, std::size_t buffer_size);

// Send all the supplied data, handling partial sends.
bool send_all(int socket_fd, const char *data, std::size_t data_size);

#endif