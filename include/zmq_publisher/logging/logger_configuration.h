#pragma once

#include <filesystem>

namespace zmq_publisher {

class LoggerConfiguration {
public:
    static void configure(const std::filesystem::path& path, int fallback_verbosity = 0);
};

}  // namespace zmq_publisher
