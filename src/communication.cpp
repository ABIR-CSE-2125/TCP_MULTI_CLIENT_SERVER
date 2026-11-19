#include "communication.hpp"
#include <cstdio>
#include <iostream>
#include <sys/socket.h>

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