#pragma once

#include "zmq_publisher/channel/channel.h"
#include "zmq_publisher/processor/processor_context.h"

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>

namespace zmq_publisher {

class ProcessorRegistry;
class ProcessorScheduler;
class ZmqTransport;
class MetricsRegistry;

class ChannelManager {
public:
    ChannelManager(
        ProcessorRegistry& registry,
        ProcessorScheduler& scheduler,
        ZmqTransport& transport,
        ProcessorContext context,
        MetricsRegistry* metrics = nullptr);

    void configure(const nlohmann::json& channels_config);
    std::size_t size() const noexcept { return channels_.size(); }

private:
    ProcessorRegistry& registry_;
    ProcessorScheduler& scheduler_;
    ZmqTransport& transport_;
    ProcessorContext context_;
    MetricsRegistry* metrics_;
    std::unordered_map<std::string, std::unique_ptr<Channel>> channels_;
};

}  // namespace zmq_publisher
