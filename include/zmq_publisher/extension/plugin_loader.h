#pragma once

#include <string>
#include <vector>

namespace zmq_publisher {

class ProcessorRegistry;

class PluginLoader {
public:
    explicit PluginLoader(ProcessorRegistry& registry);
    ~PluginLoader();

    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    void load(const std::string& library_path);

private:
    ProcessorRegistry& registry_;
    std::vector<void*> handles_;
};

}  // namespace zmq_publisher
