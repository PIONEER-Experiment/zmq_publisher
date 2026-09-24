#include "zmq_publisher/config/configuration.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace zmq_publisher {

Configuration::Configuration(std::filesystem::path source_path, nlohmann::json root)
    : source_path_(std::move(source_path)), root_(std::move(root)) {}

Configuration Configuration::from_file(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("Failed to open configuration file: " + path.string());
    }

    nlohmann::json root;
    try {
        stream >> root;
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error(
            "Failed to parse configuration file '" + path.string() + "': " + error.what());
    }
    if (!root.is_object()) {
        throw std::runtime_error("Configuration root must be an object: " + path.string());
    }
    expand_environment(root);
    return Configuration(std::filesystem::absolute(path), std::move(root));
}

void Configuration::expand_environment(nlohmann::json& value) {
    if (value.is_string()) {
        value = expand_environment(value.get<std::string>());
    } else if (value.is_array() || value.is_object()) {
        for (auto& child : value) {
            expand_environment(child);
        }
    }
}

std::string Configuration::expand_environment(std::string value) {
    std::size_t search_from = 0;
    while (true) {
        const auto dollar = value.find('$', search_from);
        if (dollar == std::string::npos || dollar + 2 >= value.size()) {
            break;
        }
        const char open = value[dollar + 1];
        const char close = open == '(' ? ')' : open == '{' ? '}' : '\0';
        if (close == '\0') {
            search_from = dollar + 1;
            continue;
        }
        const auto end = value.find(close, dollar + 2);
        if (end == std::string::npos) {
            throw std::runtime_error("Unclosed environment placeholder: " + value);
        }
        const auto name = value.substr(dollar + 2, end - dollar - 2);
        const char* replacement = std::getenv(name.c_str());
        if (!replacement) {
            throw std::runtime_error("Environment variable is not set: " + name);
        }
        value.replace(dollar, end - dollar + 1, replacement);
        search_from = dollar + std::char_traits<char>::length(replacement);
    }
    return value;
}

}  // namespace zmq_publisher
