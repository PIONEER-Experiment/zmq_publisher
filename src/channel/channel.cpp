#include "zmq_publisher/channel/channel.h"

#include "zmq_publisher/metrics/component_metrics.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace zmq_publisher {

Channel::Channel(ChannelConfig config, std::shared_ptr<ComponentMetrics> metrics)
    : config_(std::move(config)),
      metrics_(metrics ? std::move(metrics)
                       : std::make_shared<ComponentMetrics>("channel/" + config_.id)) {
    if (config_.id.empty()) {
        throw std::invalid_argument("Channel id cannot be empty");
    }
    if (config_.endpoint.empty()) {
        throw std::invalid_argument("Channel endpoint cannot be empty: " + config_.id);
    }
    if (config_.delivery_size == 0) {
        throw std::invalid_argument("Delivery size must be greater than zero: " + config_.id);
    }
}

std::vector<PublishMessage> Channel::accept(std::vector<OutputMessage> output) {
    std::vector<PublishMessage> messages;
    if (output.empty()) {
        return messages;
    }

    std::lock_guard lock(mutex_);
    metrics_->record_invocation();
    metrics_->record_items(output.size());
    for (auto& item : output) {
        metrics_->record_bytes(item.size());
        item.set_sequence(next_product_sequence_++);
    }

    if (config_.delivery_policy == DeliveryPolicy::each) {
        messages.reserve(output.size());
        for (auto& item : output) {
            std::vector<OutputMessage> product;
            product.push_back(std::move(item));
            messages.push_back(make_message(std::move(product)));
        }
        return messages;
    }

    if (config_.delivery_policy == DeliveryPolicy::latest) {
        std::vector<OutputMessage> latest;
        latest.push_back(std::move(output.back()));
        messages.push_back(make_message(std::move(latest)));
        return messages;
    }

    for (auto& item : output) {
        pending_.push_back(std::move(item));

        if (config_.delivery_policy == DeliveryPolicy::batch) {
            if (pending_.size() < config_.delivery_size) {
                continue;
            }
            std::vector<OutputMessage> batch;
            batch.reserve(config_.delivery_size);
            for (std::size_t index = 0; index < config_.delivery_size; ++index) {
                batch.push_back(std::move(pending_.front()));
                pending_.pop_front();
            }
            messages.push_back(make_message(std::move(batch)));
            continue;
        }

        while (pending_.size() > config_.delivery_size) {
            pending_.pop_front();
        }
        std::vector<OutputMessage> window;
        window.reserve(pending_.size());
        std::copy(pending_.begin(), pending_.end(), std::back_inserter(window));
        messages.push_back(make_message(std::move(window)));
    }
    return messages;
}

PublishMessage Channel::make_message(std::vector<OutputMessage> products) {
    return PublishMessage{
        config_.endpoint,
        config_.topic,
        next_publish_sequence_++,
        std::chrono::steady_clock::now(),
        std::move(products),
    };
}

}  // namespace zmq_publisher
