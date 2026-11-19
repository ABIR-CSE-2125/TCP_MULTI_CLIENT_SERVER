#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <sys/types.h>

int main()
{
    // CREATE A SOCKET
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }
    std::cout << "Socket created successfully!\n";

    // BIND THE SOCKET TO AN IP ADDRESS AND PORT
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;                                       // FOR IPV4
    server_address.sin_port = htons(8080);                                     // NETWORK BYTE ORDER FOR PORT 8080 (BIG ENDIAN)
    int ip_status = inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr); // CONVERT IP ADDRESS FROM TEXT TO BINARY FORM

    /*{
    ip_status --> 1 : The conversion was successful, and the IP address is valid.
    ip_status --> 0 : The input string is not a valid IP address.
    ip_status --> -1 : An System error occurred during the conversion
    }*/
    if (ip_status < 0)
    {
        perror("Invalid address or address not supported");
        close(socket_fd);
        return 1;
    }
    else if (ip_status == 0)
    {
        std::cerr << "Invalid IP address format\n";
        close(socket_fd);
        return 1;
    }
    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        close(socket_fd);
        return 1;
    }
    std::cout << "Socket bound to 127.0.0.1:8080\n";

    // LISTEN FOR INCOMING CONNECTIONS
    if (listen(socket_fd, 5) < 0)
    {
        perror("Listen failed");
        close(socket_fd);
        return 1;
    }
    std::cout << "Server is listening on 127.0.0.1:8080\n";

    // getchar(); // WAIT FOR USER INPUT TO CLOSE THE SERVER

    // Accepting a connection (optional, for demonstration purposes)
    sockaddr_in client_address{};
    socklen_t client_address_len = sizeof(client_address);
    int client_socket_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket_fd < 0)
    {
        perror("Accept failed");
        close(socket_fd);
        return 1;
    }
        std::cout << "Accepted a connection from "
                  << inet_ntoa(client_address.sin_addr) << ":"
                  << ntohs(client_address.sin_port) << "\n";

    while (true)
    {
        // READ BLOCK
        char buffer[1024] ;
        ssize_t bytes_received = recv(client_socket_fd, buffer, sizeof(buffer), 0);
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
        std::cout << "Received: " << bytes_received << " bytes\n" << "Content: ";
        std::cout.write(buffer, bytes_received);
        std::cout << '\n';
        // SEND BLOCK
        size_t total_bytes_sent = 0;
        // size_t send_window = 24;
        while (total_bytes_sent < static_cast<size_t>(bytes_received))
        {
            ssize_t bytes_sent = send(client_socket_fd,
                                      buffer + total_bytes_sent,
                                      static_cast<size_t>(bytes_received) - total_bytes_sent,
                                    MSG_NOSIGNAL);
            if (bytes_sent < 0)
            {
                perror("Send failed");
                break;
            }
            else if (bytes_sent == 0)
            {
                std::cerr << "Send made no progress\n";
                break;
            }
            total_bytes_sent += static_cast<size_t>(bytes_sent);
            std::cout << "Sent: " << bytes_sent << " bytes\n";
        }
        if (total_bytes_sent < static_cast<size_t>(bytes_received))
        {
            break;
        }
    }

    close(client_socket_fd);
    close(socket_fd);
    return 0;
}
