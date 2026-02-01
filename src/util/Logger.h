#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

#include <memory>
#include <string>

namespace ms
{

/**
 * @brief Logger utility class using spdlog
 *
 * Provides pretty colored logging with different log levels.
 * Usage:
 *   Logger::Info("Message");
 *   Logger::Debug("Value: {}", value);
 *   Logger::Error("Error: {}", error);
 */
class Logger
{
public:
    /**
     * @brief Initialize the logger
     * @param name Logger name (appears in log output)
     * @param level Minimum log level (default: debug in debug builds, info in release)
     */
    static void Initialize(const std::string& name = "MapleStory",
                          spdlog::level::level_enum level = spdlog::level::debug);

    /**
     * @brief Shutdown the logger
     */
    static void Shutdown();

    /**
     * @brief Set log level
     */
    static void SetLevel(spdlog::level::level_enum level);

    /**
     * @brief Get the underlying spdlog logger
     */
    static auto Get() -> std::shared_ptr<spdlog::logger>&;

    // Convenience logging functions
    template <typename... Args>
    static void Trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        Get()->critical(fmt, std::forward<Args>(args)...);
    }

    // Simple string overloads
    static void Trace(const std::string& msg) { Get()->trace(msg); }
    static void Debug(const std::string& msg) { Get()->debug(msg); }
    static void Info(const std::string& msg) { Get()->info(msg); }
    static void Warn(const std::string& msg) { Get()->warn(msg); }
    static void Error(const std::string& msg) { Get()->error(msg); }
    static void Critical(const std::string& msg) { Get()->critical(msg); }

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

// Convenience macros for file/line info (optional usage)
#define LOG_TRACE(...) ::ms::Logger::Trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::ms::Logger::Debug(__VA_ARGS__)
#define LOG_INFO(...)  ::ms::Logger::Info(__VA_ARGS__)
#define LOG_WARN(...)  ::ms::Logger::Warn(__VA_ARGS__)
#define LOG_ERROR(...) ::ms::Logger::Error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::ms::Logger::Critical(__VA_ARGS__)

} // namespace ms
