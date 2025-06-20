#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>
#include <string>

namespace utils {

class LoggerConfig {
public:
    static void ConfigureFromFile(const std::string& filename);
    static void ConfigureFromFile();  // no-arg overload

private:
    static spdlog::level::level_enum parseLevel(const std::string& levelStr);
};

} // namespace utils
