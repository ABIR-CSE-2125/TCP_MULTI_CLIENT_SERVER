#include "logger.hpp"

int main()
{
    Logger logger(LogLevel::ERROR);

    logger.log(LogLevel::INFO, "SERVER", "Server started");
    logger.log(LogLevel::DEBUG, "THREAD_POOL", "Worker started");
    logger.log(LogLevel::ERROR, "SERVER", "Socket creation failed");

    return 0;
}