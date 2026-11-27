
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>

enum class LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger
{
private:
    std::mutex log_mutex;
    LogLevel minimum_level;

public:
    explicit Logger(LogLevel level = LogLevel::INFO);

    void log(
        LogLevel level,
        const std::string &component,
        const std::string &message);
};

#endif
