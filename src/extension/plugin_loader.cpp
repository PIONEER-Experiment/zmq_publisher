#include "zmq_publisher/extension/plugin_loader.h"

#include "zmq_publisher/processor/processor_registry.h"

#include <dlfcn.h>
#include <stdexcept>

namespace zmq_publisher {

namespace {
using RegisterProcessors = void (*)(ProcessorRegistry&);
constexpr auto kRegistrationSymbol = "zmq_publisher_register_processors";
}

PluginLoader::PluginLoader(ProcessorRegistry& registry) : registry_(registry) {}

PluginLoader::~PluginLoader() {
    // Registry entries contain std::function objects whose code and destructors
    // live in the extension. Keep successful extensions resident until process
    // exit; the operating system releases them after all C++ objects are gone.
    handles_.clear();
}

void PluginLoader::load(const std::string& library_path) {
    dlerror();
    void* handle = dlopen(library_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char* error = dlerror();
        throw std::runtime_error(
            "Failed to load processor extension '" + library_path + "': " +
            (error ? error : "unknown loader error"));
    }

    dlerror();
    auto registration = reinterpret_cast<RegisterProcessors>(
        dlsym(handle, kRegistrationSymbol));
    if (const char* error = dlerror()) {
        dlclose(handle);
        throw std::runtime_error(
            "Processor extension '" + library_path +
            "' does not export " + kRegistrationSymbol + ": " + error);
    }

    try {
        registration(registry_);
        handles_.push_back(handle);
    } catch (...) {
        dlclose(handle);
        throw;
    }
}

}  // namespace zmq_publisher
