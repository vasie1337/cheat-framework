#include "logger.hpp"

#include <windows.h>
#include <iostream>

Logger::~Logger()
{
    if (m_file)
    {
        fclose(m_file);
    }
}

void Logger::initialize(LoggerBackend backend, LogLevel level, const std::string &filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_backend = backend;
    m_level = level;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    if (backend == LoggerBackend::File || backend == LoggerBackend::Both)
    {
        if (!filename.empty())
        {
#pragma warning(suppress : 4996)
            m_file = fopen(filename.c_str(), "a");
            if (!m_file)
            {
                std::cerr << "Failed to open log file: " << filename << std::endl;
                m_backend = LoggerBackend::Console;
            }
        }
    }
}

void Logger::debug(const char *format, ...)
{
    if (m_level > LogLevel::Debug)
        return;

    va_list args;
    va_start(args, format);
    log_internal(LogLevel::Debug, format, args);
    va_end(args);
}

void Logger::info(const char *format, ...)
{
    if (m_level > LogLevel::Info)
        return;

    va_list args;
    va_start(args, format);
    log_internal(LogLevel::Info, format, args);
    va_end(args);
}

void Logger::warning(const char *format, ...)
{
    if (m_level > LogLevel::Warning)
        return;

    va_list args;
    va_start(args, format);
    log_internal(LogLevel::Warning, format, args);
    va_end(args);
}

void Logger::error(const char *format, ...)
{
    if (m_level > LogLevel::Error)
        return;

    va_list args;
    va_start(args, format);
    log_internal(LogLevel::Error, format, args);
    va_end(args);
}

void Logger::critical(const char *format, ...)
{
    if (m_level > LogLevel::Critical)
        return;

    va_list args;
    va_start(args, format);
    log_internal(LogLevel::Critical, format, args);
    va_end(args);
}

void Logger::log_internal(LogLevel level, const char *format, va_list args)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);

    std::string timestamp = get_timestamp();
    const char *level_str = get_level_string(level);
    const char *color = get_level_color(level);

    if (m_backend == LoggerBackend::Console || m_backend == LoggerBackend::Both)
    {
        int level_padding = 8 - static_cast<int>(strlen(level_str));

        printf("%s[%s]%s%*s%s[%s]%s %s\n",
               BRIGHT_BLACK, timestamp.c_str(),
               RESET, level_padding, "",
               color, level_str,
               RESET, buffer);
        fflush(stdout);
    }

    if (m_file && (m_backend == LoggerBackend::File || m_backend == LoggerBackend::Both))
    {
        fprintf(m_file, "[%-12s] [%-8s] %s\n", timestamp.c_str(), level_str, buffer);
        fflush(m_file);
    }
}

std::string Logger::get_timestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::tm tm;
    localtime_s(&tm, &time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

const char *Logger::get_level_string(LogLevel level) const
{
    switch (level)
    {
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

const char *Logger::get_level_color(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::Debug:
        return BRIGHT_CYAN;
    case LogLevel::Info:
        return BRIGHT_GREEN;
    case LogLevel::Warning:
        return BRIGHT_YELLOW;
    case LogLevel::Error:
        return BRIGHT_RED;
    case LogLevel::Critical:
        return BRIGHT_MAGENTA;
    default:
        return WHITE;
    }
}
