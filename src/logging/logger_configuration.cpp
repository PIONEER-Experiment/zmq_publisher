#include "zmq_publisher/logging/logger_configuration.h"

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace zmq_publisher {

namespace {
spdlog::level::level_enum parse_level(const std::string& level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "warn") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    if (level == "off") return spdlog::level::off;
    return spdlog::level::info;
}
}

void LoggerConfiguration::configure(
    const std::filesystem::path& path,
    int fallback_verbosity) {
    try {
        std::ifstream stream(path);
        if (!stream) {
            throw std::runtime_error("cannot open file");
        }
        nlohmann::json root;
        stream >> root;
        const auto config = root.at("logger");
        const auto level = parse_level(config.value("level", "info"));
        std::vector<spdlog::sink_ptr> sinks;

        const auto sink_config = config.value("sinks", nlohmann::json::object());
        const auto console = sink_config.value("console", nlohmann::json::object());
        if (console.value("enabled", true)) {
            spdlog::sink_ptr sink = console.value("color", true)
                ? std::static_pointer_cast<spdlog::sinks::sink>(
                    std::make_shared<spdlog::sinks::stdout_color_sink_mt>())
                : std::static_pointer_cast<spdlog::sinks::sink>(
                    std::make_shared<spdlog::sinks::stdout_sink_mt>());
            sink->set_level(level);
            sinks.push_back(std::move(sink));
        }

        const auto file = sink_config.value("file", nlohmann::json::object());
        if (file.value("enabled", false)) {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                file.value("filename", "logs/zmq_publisher.log"),
                file.value("max_size", 10485760U),
                file.value("max_files", 3U));
            sink->set_level(level);
            sinks.push_back(std::move(sink));
        }
        if (sinks.empty()) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }

        auto logger = std::make_shared<spdlog::logger>(
            config.value("name", "zmq_publisher"), sinks.begin(), sinks.end());
        logger->set_level(level);
        logger->set_pattern(config.value(
            "pattern", "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"));
        spdlog::set_default_logger(std::move(logger));
        spdlog::flush_on(spdlog::level::info);
    } catch (const std::exception& error) {
        spdlog::set_level(fallback_verbosity > 1
            ? spdlog::level::debug
            : fallback_verbosity > 0 ? spdlog::level::info : spdlog::level::warn);
        spdlog::warn("Using default logger configuration; {}: {}", path.string(), error.what());
    }
}

}  // namespace zmq_publisher
