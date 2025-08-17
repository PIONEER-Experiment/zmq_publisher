#include "utilities/LoggerConfig.h"
#include <filesystem>
#include <fstream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace utils {

spdlog::level::level_enum LoggerConfig::parseLevel(const std::string& levelStr) {
    if (levelStr == "trace") return spdlog::level::trace;
    if (levelStr == "debug") return spdlog::level::debug;
    if (levelStr == "info")  return spdlog::level::info;
    if (levelStr == "warn")  return spdlog::level::warn;
    if (levelStr == "error") return spdlog::level::err;
    if (levelStr == "critical") return spdlog::level::critical;
    if (levelStr == "off")   return spdlog::level::off;
    return spdlog::level::info;  // default
}

void LoggerConfig::ConfigureFromFile() {
    // Resolve config/logger_config.json relative to this source file
    std::filesystem::path sourcePath(__FILE__);
    auto configPath = sourcePath.parent_path().parent_path().parent_path() / "config" / "logger_config.json";
    auto canonicalPath = std::filesystem::canonical(configPath);
    ConfigureFromFile(canonicalPath.string());
}

void LoggerConfig::ConfigureFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            spdlog::error("LoggerConfig: Failed to open config file '{}'", filename);
            return;
        }

        nlohmann::json config;
        file >> config;

        auto loggerConfig = config.at("logger");

        std::string loggerName = loggerConfig.value("name", "app_logger");
        std::string levelStr = loggerConfig.value("level", "info");
        std::string pattern = loggerConfig.value("pattern", "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        auto level = parseLevel(levelStr);

        std::vector<spdlog::sink_ptr> sinks;

        if (loggerConfig.contains("sinks")) {
            auto sinksConfig = loggerConfig.at("sinks");

            // Console sink
            if (sinksConfig.contains("console")) {
                auto consoleConfig = sinksConfig.at("console");
                bool enabled = consoleConfig.value("enabled", true);
                bool color = consoleConfig.value("color", true);
                if (enabled) {
                    spdlog::sink_ptr sink;
                    if (color) {
                        sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    } else {
                        sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
                    }
                    sink->set_level(level);
                    sinks.push_back(sink);
                }
            }

            // Rotating file sink
            if (sinksConfig.contains("file")) {
                auto fileConfig = sinksConfig.at("file");
                bool enabled = fileConfig.value("enabled", false);
                if (enabled) {
                    std::string file_path = fileConfig.value("filename", "logs/app.log");
                    size_t max_size = fileConfig.value("max_size", 10485760); // 10 MB
                    size_t max_files = fileConfig.value("max_files", 3);
                    try {
                        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                            file_path, max_size, max_files);
                        sink->set_level(level);
                        sinks.push_back(sink);
                    } catch (const spdlog::spdlog_ex& ex) {
                        spdlog::error("LoggerConfig: Failed to create file sink: {}", ex.what());
                    }
                }
            }
        }

        if (sinks.empty()) {
            // fallback to console with color
            auto fallbackSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            fallbackSink->set_level(level);
            sinks.push_back(fallbackSink);
        }

        auto logger = std::make_shared<spdlog::logger>(loggerName, sinks.begin(), sinks.end());
        logger->set_level(level);
        logger->set_pattern(pattern);

        spdlog::set_default_logger(logger);
        spdlog::set_level(level);              // ✅ Global filter level
        spdlog::flush_on(spdlog::level::info);

        spdlog::info("LoggerConfig: Logger '{}' initialized with level '{}'", loggerName, levelStr);
    } catch (const std::exception& e) {
        spdlog::error("LoggerConfig: Exception caught during logger configuration: {}", e.what());
    }
}


} // namespace utils
