#include "zmq_publisher/app/application.h"
#include "zmq_publisher/config/configuration.h"
#include "zmq_publisher/logging/logger_configuration.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace {

struct Arguments {
    std::filesystem::path config = "config/config.json";
    std::filesystem::path logger_config = "config/logger_config.json";
};

Arguments parse_arguments(int argc, char* argv[]) {
    Arguments result;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if ((argument == "-c" || argument == "--config") && index + 1 < argc) {
            result.config = argv[++index];
        } else if (argument == "--logger-config" && index + 1 < argc) {
            result.logger_config = argv[++index];
        } else if (argument == "-h" || argument == "--help") {
            std::cout << "Usage: zmq_publisher [--config PATH] [--logger-config PATH]\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown or incomplete argument: " + argument);
        }
    }
    return result;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const auto arguments = parse_arguments(argc, argv);
        const auto configuration =
            zmq_publisher::Configuration::from_file(arguments.config);
        const auto& root = configuration.root();
        const auto settings = root.value("general-settings", nlohmann::json::object());
        const int verbosity = settings.value("verbose", 0);
        zmq_publisher::LoggerConfiguration::configure(arguments.logger_config, verbosity);

        zmq_publisher::Application application(root);
        return application.run();
    } catch (const std::exception& error) {
        spdlog::critical("ZMQ publisher failed: {}", error.what());
        return 1;
    }
}
