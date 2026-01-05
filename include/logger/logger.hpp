/**
 * @file logger.hpp
 * @brief Professional-grade, lightweight, thread-safe header-only logging
 * library for C++17.
 */

#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Logger {

    namespace fs = std::filesystem;

    /**
     * @brief Log levels
     */
    enum class LogLevel {
        Debug = 0,
        Info,
        Warning,
        Error,
        Critical,
        None  // Used to disable all logging
    };

    inline std::string_view level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:
                return "DEBUG";
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warning:
                return "WARNING";
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Critical:
                return "CRITICAL";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * @brief Information about the source code location
     */
    struct SourceLocation {
        std::string_view file;
        int line;
        std::string_view function;

        std::string_view filename_only() const {
            auto pos = file.find_last_of("\\/");
            if (pos == std::string_view::npos) return file;
            return file.substr(pos + 1);
        }
    };

    /**
     * @brief Data structure passed to sinks
     */
    struct LogDetails {
        LogLevel level;
        std::string message;
        std::string timestamp;
        SourceLocation location;
        std::chrono::system_clock::time_point time;
    };

    /**
     * @brief Abstract base class for all log sinks
     */
    class Sink {
       public:
        virtual ~Sink() = default;
        virtual void log(const LogDetails& details) = 0;
        virtual void set_level(LogLevel level) {
            level_ = level;
        }
        LogLevel get_level() const {
            return level_;
        }

       protected:
        LogLevel level_ = LogLevel::Debug;
    };

    /**
     * @brief Console sink with ANSI color support
     */
    class ConsoleSink : public Sink {
       public:
        void log(const LogDetails& details) override {
            if (details.level < level_) return;

            std::ostream& os = (details.level >= LogLevel::Error) ? std::cerr : std::cout;

            os << "[" << details.timestamp << "] " << colorize(details.level) << "["
               << level_to_string(details.level) << "]" << reset() << " " << details.message;

            if (details.level >= LogLevel::Warning) {
                os << " (" << details.location.filename_only() << ":" << details.location.line
                   << ")";
            }
            os << std::endl;
        }

       private:
        static const char* colorize(LogLevel level) {
            switch (level) {
                case LogLevel::Debug:
                    return "\033[36m";  // Cyan
                case LogLevel::Info:
                    return "\033[32m";  // Green
                case LogLevel::Warning:
                    return "\033[33m";  // Yellow
                case LogLevel::Error:
                    return "\033[31m";  // Red
                case LogLevel::Critical:
                    return "\033[1;31m";  // Bold Red
                default:
                    return "\033[0m";
            }
        }

        static const char* reset() {
            return "\033[0m";
        }
    };

    /**
     * @brief Simple File Sink
     */
    class FileSink : public Sink {
       public:
        explicit FileSink(std::string_view filename) : filename_(filename) {
            open_file();
        }

        void log(const LogDetails& details) override {
            if (!file_.is_open() || details.level < level_) return;

            file_ << "[" << details.timestamp << "] "
                  << "[" << level_to_string(details.level) << "] " << details.message << " ("
                  << details.location.filename_only() << ":" << details.location.line << ")"
                  << std::endl;
        }

       protected:
        void open_file() {
            try {
                if (file_.is_open()) file_.close();

                // Ensure directory exists
                fs::path p(filename_);
                if (p.has_parent_path() && !fs::exists(p.parent_path())) {
                    fs::create_directories(p.parent_path());
                }

                file_.open(filename_, std::ios::out | std::ios::app);
            } catch (const std::exception& e) {
                std::cerr << "Logger Error: Failed to open file " << filename_ << ": " << e.what()
                          << std::endl;
            }
        }

        std::string filename_;
        std::ofstream file_;
    };

    /**
     * @brief Rotating File Sink
     */
    class RotatingFileSink : public FileSink {
       public:
        RotatingFileSink(std::string_view filename, size_t max_size_bytes, int max_files)
            : FileSink(filename), max_size_(max_size_bytes), max_files_(max_files) {}

        void log(const LogDetails& details) override {
            if (details.level < level_) return;

            if (should_rotate()) {
                rotate();
            }

            FileSink::log(details);
        }

       private:
        bool should_rotate() {
            if (!file_.is_open()) return false;
            std::error_code ec;
            auto size = fs::file_size(filename_, ec);
            if (ec) return false;
            return size >= max_size_;
        }

        void rotate() {
            file_.close();
            std::error_code ec;

            for (int i = max_files_ - 1; i > 0; --i) {
                std::string old_name = get_rotated_filename(i);
                std::string new_name = get_rotated_filename(i + 1);
                if (fs::exists(old_name, ec)) {
                    fs::rename(old_name, new_name, ec);
                }
            }

            if (fs::exists(filename_, ec)) {
                fs::rename(filename_, get_rotated_filename(1), ec);
            }

            open_file();
        }

        std::string get_rotated_filename(int index) {
            fs::path p(filename_);
            std::string ext = p.extension().string();
            std::string stem = p.stem().string();
            p.replace_filename(stem + "." + std::to_string(index) + ext);
            return p.string();
        }

        size_t max_size_;
        int max_files_;
    };

    /**
     * @brief Main Logger class (Singleton)
     */
    class Logger {
       public:
        static Logger& instance() {
            static Logger logger;
            return logger;
        }

        // Disable copying
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            sinks_.clear();
            global_level_ = LogLevel::Info;
            sinks_.push_back(std::make_shared<ConsoleSink>());
        }

        void add_sink(std::shared_ptr<Sink> sink) {
            std::lock_guard<std::mutex> lock(mutex_);
            sinks_.push_back(std::move(sink));
        }

        void clear_sinks() {
            std::lock_guard<std::mutex> lock(mutex_);
            sinks_.clear();
        }

        void set_level(LogLevel level) {
            global_level_ = level;
        }

        template <typename... Args>
        void log(LogLevel level, SourceLocation loc, Args&&... args) {
            if (level < global_level_) return;

            std::lock_guard<std::mutex> lock(mutex_);

            LogDetails details;
            details.level = level;
            details.location = loc;
            details.time = std::chrono::system_clock::now();
            details.timestamp = get_timestamp(details.time);

            std::ostringstream oss;
            (oss << ... << std::forward<Args>(args));
            details.message = std::move(oss).str();

            for (auto& sink : sinks_) {
                sink->log(details);
            }
        }

       private:
        Logger() {
            sinks_.push_back(std::make_shared<ConsoleSink>());
        }

        static std::string get_timestamp(std::chrono::system_clock::time_point now) {
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
                1000;

            struct tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm_buf, &time);
#else
            localtime_r(&time, &tm_buf);
#endif

            std::ostringstream oss;
            oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
            oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        std::vector<std::shared_ptr<Sink>> sinks_;
        LogLevel global_level_ = LogLevel::Info;
        std::mutex mutex_;
    };

}  // namespace Logger

#define LOGGER_LOC                   \
    Logger::SourceLocation {         \
        __FILE__, __LINE__, __func__ \
    }

#define LOG_DEBUG(...) \
    Logger::Logger::instance().log(Logger::LogLevel::Debug, LOGGER_LOC, __VA_ARGS__)
#define LOG_INFO(...) \
    Logger::Logger::instance().log(Logger::LogLevel::Info, LOGGER_LOC, __VA_ARGS__)
#define LOG_WARN(...) \
    Logger::Logger::instance().log(Logger::LogLevel::Warning, LOGGER_LOC, __VA_ARGS__)
#define LOG_ERROR(...) \
    Logger::Logger::instance().log(Logger::LogLevel::Error, LOGGER_LOC, __VA_ARGS__)
#define LOG_CRITICAL(...) \
    Logger::Logger::instance().log(Logger::LogLevel::Critical, LOGGER_LOC, __VA_ARGS__)
