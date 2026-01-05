#include <logger/logger.hpp>

int main() {
    auto& logger = Logger::Logger::instance();
    logger.set_level(Logger::LogLevel::Debug);

    LOG_INFO("Hello, Professional World!");

    // 1. Logging to a specific directory
    // You can use relative or absolute paths.
    // Make sure the directory exists!
    auto file_sink = std::make_shared<Logger::FileSink>("logs/app_simple.log");
    logger.add_sink(file_sink);

    // 2. Logging with Rotation (Limits)
    // filename, max_size (5KB for demo), max_files (3)
    auto rotating_sink =
        std::make_shared<Logger::RotatingFileSink>("logs/rotating.log", 5 * 1024, 3);
    logger.add_sink(rotating_sink);

    for (int i = 0; i < 100; ++i) {
        LOG_DEBUG("Iteration ", i, ": Writing some data to test rotation logic...");
    }

    LOG_INFO("Check the 'logs' directory to see the results!");

    return 0;
}