#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>
#include <mutex>
#include <memory>
#include <chrono>
#include <ctime>

enum class LoggerBackend {
    Console,
    File,
    Both
};

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void initialize(LoggerBackend backend, LogLevel level, const std::string& filename = "");
    void set_level(LogLevel level) { m_level = level; }
    
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    void critical(const char* format, ...);

private:
    Logger() = default;
    ~Logger();
    
    void log_internal(LogLevel level, const char* format, va_list args);
    std::string get_timestamp() const;
    const char* get_level_string(LogLevel level) const;
    const char* get_level_color(LogLevel level) const;
    
    std::mutex m_mutex;
    LoggerBackend m_backend = LoggerBackend::Console;
    LogLevel m_level = LogLevel::Debug;
    FILE* m_file = nullptr;
    
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BLACK = "\033[30m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";
    static constexpr const char* BRIGHT_BLACK = "\033[90m";
    static constexpr const char* BRIGHT_RED = "\033[91m";
    static constexpr const char* BRIGHT_GREEN = "\033[92m";
    static constexpr const char* BRIGHT_YELLOW = "\033[93m";
    static constexpr const char* BRIGHT_BLUE = "\033[94m";
    static constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    static constexpr const char* BRIGHT_CYAN = "\033[96m";
    static constexpr const char* BRIGHT_WHITE = "\033[97m";
};

#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)

