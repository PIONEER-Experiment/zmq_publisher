#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>

namespace zmq_publisher {

class Configuration {
public:
    static Configuration from_file(const std::filesystem::path& path);

    const nlohmann::json& root() const noexcept { return root_; }
    const std::filesystem::path& source_path() const noexcept { return source_path_; }

private:
    Configuration(std::filesystem::path source_path, nlohmann::json root);

    static void expand_environment(nlohmann::json& value);
    static std::string expand_environment(std::string value);

    std::filesystem::path source_path_;
    nlohmann::json root_;
};

}  // namespace zmq_publisher
