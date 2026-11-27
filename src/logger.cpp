#include "logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace
{

    std::string level_to_string(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::WARN:
            return "WARN";
        default:
            return "UNKNOWN";
        }
    }

    std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t current_time =
            std::chrono::system_clock::to_time_t(now);
        std::tm local_time{};
        localtime_r(&current_time, &local_time);
        std::ostringstream timestamp;

        timestamp << std::put_time(
            &local_time,
            "%Y-%m-%d %H:%M:%S");
        return timestamp.str();
    }
}

Logger::Logger(LogLevel level)
    : minimum_level(level)
{
}

void Logger::log(
    LogLevel level,
    const std::string &component,
    const std::string &message)
{
    if (minimum_level > level)
        return;
    std::lock_guard<std::mutex> lock(log_mutex);

    std::string timestamp_string = get_timestamp();

    std::cout << "[" << timestamp_string << "]\t"
              << "[" << level_to_string(level) << "]\t"
              << "[" << component << "]\t"
              << message << "\n";
}