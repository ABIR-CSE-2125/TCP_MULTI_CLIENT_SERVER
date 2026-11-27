
#include "config.hpp"
#include "logger.hpp"
//===============================================
#include <arpa/inet.h>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace
{
    // Remove leading and trailing whitespace
    std::string trim(const std::string &str)
    {
        std::size_t start = str.find_first_not_of(" \t\r\n");

        if (start == std::string::npos)
            return "";

        std::size_t end = str.find_last_not_of(" \t\r\n");

        return str.substr(start, end - start + 1);
    }

    // Convert a string to an integer strictly
    bool parse_integer(const std::string &value, int &result)
    {
        try
        {
            std::size_t processed = 0;
            result = std::stoi(value, &processed);

            return processed == value.length();
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

    LogLevel string_to_log_level(const std::string &level)
    {
        if (level == "DEBUG")
            return LogLevel::DEBUG;

        if (level == "INFO")
            return LogLevel::INFO;

        if (level == "WARN")
            return LogLevel::WARN;

        if (level == "ERROR")
            return LogLevel::ERROR;

        throw std::invalid_argument("Invalid LOG_LEVEL: " + level);
    }
}

bool load_config(const std::string &file_path, ServerConfig &config)
{
    std::ifstream file(file_path);

    if (!file.is_open())
    {
        std::cerr << "Could not open configuration file: "
                  << file_path << '\n';
        return false;
    }

    std::map<std::string, std::string> values;

    std::string line;
    int line_number = 0;

    while (std::getline(file, line))
    {
        ++line_number;

        line = trim(line);

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        std::size_t separator = line.find('=');

        if (separator == std::string::npos)
        {
            std::cerr << "Invalid configuration at line "
                      << line_number << ": expected KEY=VALUE\n";
            return false;
        }

        std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));

        if (key.empty() || value.empty())
        {
            std::cerr << "Empty key or value at line "
                      << line_number << '\n';
            return false;
        }

        if (values.find(key) != values.end())
        {
            std::cerr << "Duplicate configuration key: "
                      << key << '\n';
            return false;
        }

        values[key] = value;
    }

    // Check whether all required keys are present
    const std::string required_keys[] = {
        "SERVER_IP",
        "SERVER_PORT",
        "LISTEN_BACKLOG",
        "BUFFER_SIZE",
        "NUM_THREAD",
        "LOG_LEVEL"};

    for (const auto &key : required_keys)
    {
        if (values.find(key) == values.end())
        {
            std::cerr << "Missing configuration key: "
                      << key << '\n';
            return false;
        }
    }

    // Reject unknown keys
    for (const auto &entry : values)
    {
        bool known = false;

        for (const auto &key : required_keys)
        {
            if (entry.first == key)
            {
                known = true;
                break;
            }
        }

        if (!known)
        {
            std::cerr << "Unknown configuration key: "
                      << entry.first << '\n';
            return false;
        }
    }

    // Validate IP address
    in_addr address{};

    if (inet_pton(AF_INET, values["SERVER_IP"].c_str(), &address) != 1)
    {
        std::cerr << "Invalid SERVER_IP\n";
        return false;
    }
    int port;
    int backlog;
    int buffer_size;
    int num_thread;
    if (
        !parse_integer(values["SERVER_PORT"], port) ||
        !parse_integer(values["LISTEN_BACKLOG"], backlog) ||
        !parse_integer(values["BUFFER_SIZE"], buffer_size) ||
        !parse_integer(values["NUM_THREAD"], num_thread))
    {
        std::cerr << "Invalid integer configuration value\n";
        return false;
    }

    // Validate numeric ranges
    if (port < 1 || port > 65535)
    {
        std::cerr << "SERVER_PORT must be between 1 and 65535\n";
        return false;
    }

    if (backlog < 1)
    {
        std::cerr << "LISTEN_BACKLOG must be positive\n";
        return false;
    }

    if (buffer_size < 1)
    {
        std::cerr << "BUFFER_SIZE must be positive\n";
        return false;
    }
    if (num_thread < 1)
    {
        std::cerr << "NUM_THREAD must be positive\n";
        return false;
    }

    // Populate the configuration structure
    config.server_ip = values["SERVER_IP"];
    config.server_port = port;
    config.listen_backlog = backlog;
    config.buffer_size = buffer_size;
    config.num_thread = num_thread;
    LogLevel log_level;
    try
    {
        log_level = string_to_log_level(values["LOG_LEVEL"]);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
    config.log_level = log_level;

    return true;
}
