#pragma once

#include "zmq_publisher/channel/channel_manager.h"
#include "zmq_publisher/extension/plugin_loader.h"
#include "zmq_publisher/metrics/metrics_registry.h"
#include "zmq_publisher/processor/processor_registry.h"
#include "zmq_publisher/scheduler/processor_scheduler.h"
#include "zmq_publisher/transport/zmq_transport.h"
#include "zmq_publisher/utility/signal_handler.h"

#include <cstddef>
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace zmq_publisher {

class Application {
public:
    explicit Application(const nlohmann::json& configuration);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    // Construct first so every MIDAS, scheduler, transport, and sidecar thread
    // inherits the blocked stop-signal mask.
    SignalHandler signals_;
    std::size_t worker_count_;
    MetricsRegistry metrics_;
    std::unique_ptr<class MetricsReporter> metrics_reporter_;
    ProcessorRegistry registry_;
    PluginLoader plugin_loader_;
    ZmqTransport transport_;
    ProcessorScheduler scheduler_;
    ChannelManager channels_;
};

}  // namespace zmq_publisher
