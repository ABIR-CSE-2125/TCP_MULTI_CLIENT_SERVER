#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

struct ServerConfig
{
    std::string server_ip;
    int server_port;
    int listen_backlog;
    int buffer_size;
    int num_thread;
};

bool load_config(const std::string &file_path, ServerConfig &config);

#endif