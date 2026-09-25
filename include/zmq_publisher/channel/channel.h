#pragma once

#include "zmq_publisher/channel/channel_config.h"
#include "zmq_publisher/processor/output_message.h"
#include "zmq_publisher/transport/publish_message.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace zmq_publisher {

class ComponentMetrics;

class Channel {
public:
    explicit Channel(ChannelConfig config, std::shared_ptr<ComponentMetrics> metrics = nullptr);

    std::vector<PublishMessage> accept(std::vector<OutputMessage> output);
    const ChannelConfig& config() const noexcept { return config_; }

private:
    PublishMessage make_message(std::vector<OutputMessage> products);

    ChannelConfig config_;
    std::deque<OutputMessage> pending_;
    std::uint64_t next_product_sequence_ = 1;
    std::uint64_t next_publish_sequence_ = 1;
    std::mutex mutex_;
    std::shared_ptr<ComponentMetrics> metrics_;
};

}  // namespace zmq_publisher
