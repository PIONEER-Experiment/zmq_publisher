#include "zmq_publisher/channel/channel.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace zmq_publisher {

Channel::Channel(ChannelConfig config)
    : config_(std::move(config)), buffer_(config_.buffer_capacity) {
    if (config_.id.empty()) {
        throw std::invalid_argument("Channel id cannot be empty");
    }
    if (config_.endpoint.empty()) {
        throw std::invalid_argument("Channel endpoint cannot be empty: " + config_.id);
    }
    if (config_.publishes_per_batch == 0) {
        throw std::invalid_argument("publishes-per-batch must be greater than zero: " + config_.id);
    }
}

std::optional<PublishMessage> Channel::accept(std::vector<std::string> output) {
    if (output.empty()) {
        return std::nullopt;
    }

    std::lock_guard lock(mutex_);
    for (auto& item : output) {
        buffer_.push(std::move(item));
    }

    if (ignored_batches_remaining_ > 0) {
        --ignored_batches_remaining_;
        return std::nullopt;
    }

    auto payload = nlohmann::json(buffer_.snapshot()).dump();
    ++published_batches_;
    if (published_batches_ % config_.publishes_per_batch == 0) {
        ignored_batches_remaining_ = config_.publishes_ignored_after_batch;
    }

    return PublishMessage{config_.endpoint, config_.topic, std::move(payload)};
}

}  // namespace zmq_publisher
