#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

namespace ms
{

std::shared_ptr<spdlog::logger> Logger::s_logger;

void Logger::Initialize(const std::string& name, spdlog::level::level_enum level)
{
    // Create sinks
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink with colors
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);

    // Custom pattern: [time] [level] [logger] message
    // Colors: trace=white, debug=cyan, info=green, warn=yellow, error=red, critical=bold red
    consoleSink->set_pattern("%^[%T] [%l] %n: %v%$");

    sinks.push_back(consoleSink);

    // Optional: File sink for persistent logs
    // auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/maplestory.log", true);
    // fileSink->set_level(spdlog::level::trace);
    // fileSink->set_pattern("[%Y-%m-%d %T.%e] [%l] [%n] %v");
    // sinks.push_back(fileSink);

    // Create logger with multiple sinks
    s_logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    s_logger->set_level(level);
    s_logger->flush_on(spdlog::level::warn);

    // Register as default logger
    spdlog::set_default_logger(s_logger);

    s_logger->info("Logger initialized");
}

void Logger::Shutdown()
{
    if (s_logger)
    {
        s_logger->info("Logger shutting down");
        s_logger->flush();
    }
    spdlog::shutdown();
}

void Logger::SetLevel(spdlog::level::level_enum level)
{
    if (s_logger)
    {
        s_logger->set_level(level);
    }
}

auto Logger::Get() -> std::shared_ptr<spdlog::logger>&
{
    if (!s_logger)
    {
        // Auto-initialize if not already done
        Initialize();
    }
    return s_logger;
}

} // namespace ms
