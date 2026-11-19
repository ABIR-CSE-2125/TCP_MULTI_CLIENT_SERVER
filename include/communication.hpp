#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <cstddef>
#include <sys/types.h>

// Receive data from a connected client.
ssize_t receive_data(int socket_fd, char *buffer, std::size_t buffer_size);

// Send all the supplied data, handling partial sends.
bool send_all(int socket_fd, const char *data, std::size_t data_size);

#endif