#include "zmq_publisher/channel/channel_manager.h"

#include "zmq_publisher/config/configuration_view.h"
#include "zmq_publisher/metrics/metrics_registry.h"
#include "zmq_publisher/processor/processor_definition.h"
#include "zmq_publisher/processor/processor_registry.h"
#include "zmq_publisher/scheduler/processor_scheduler.h"
#include "zmq_publisher/transport/zmq_transport.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace zmq_publisher {

namespace {

ChannelConfig parse_channel_config(const std::string& id, const nlohmann::json& config) {
    if (!config.is_object()) {
        throw std::invalid_argument("Channel '" + id + "' must be an object");
    }
    for (const auto& [key, _] : config.items()) {
        if (key != "enabled" && key != "endpoint" && key != "topic" &&
            key != "delivery" && key != "processors") {
            throw std::invalid_argument(
                "Channel '" + id + "' contains unknown field '" + key + "'");
        }
    }

    ChannelConfig result;
    result.id = id;
    if (!config.contains("endpoint") || !config.at("endpoint").is_string()) {
        throw std::invalid_argument("Channel '" + id + "' requires string field 'endpoint'");
    }
    if (!config.contains("topic") || !config.at("topic").is_string()) {
        throw std::invalid_argument("Channel '" + id + "' requires string field 'topic'");
    }
    result.endpoint = config.at("endpoint").get<std::string>();
    result.topic = config.at("topic").get<std::string>();

    if (!config.contains("delivery") || !config.at("delivery").is_object()) {
        throw std::invalid_argument("Channel '" + id + "' requires object field 'delivery'");
    }
    const auto& delivery = config.at("delivery");
    for (const auto& [key, _] : delivery.items()) {
        if (key != "mode" && key != "size") {
            throw std::invalid_argument(
                "Channel '" + id + "' delivery contains unknown field '" + key + "'");
        }
    }
    if (!delivery.contains("mode") || !delivery.at("mode").is_string()) {
        throw std::invalid_argument("Channel '" + id + "' delivery requires string field 'mode'");
    }
    const auto mode = delivery.at("mode").get<std::string>();
    if (mode == "each") {
        result.delivery_policy = DeliveryPolicy::each;
    } else if (mode == "latest") {
        result.delivery_policy = DeliveryPolicy::latest;
    } else if (mode == "batch") {
        result.delivery_policy = DeliveryPolicy::batch;
    } else if (mode == "sliding-window") {
        result.delivery_policy = DeliveryPolicy::sliding_window;
    } else {
        throw std::invalid_argument("Unknown delivery mode for channel '" + id + "': " + mode);
    }

    const bool sized = result.delivery_policy == DeliveryPolicy::batch ||
        result.delivery_policy == DeliveryPolicy::sliding_window;
    if (sized) {
        if (!delivery.contains("size")) {
            throw std::invalid_argument(
                "Channel '" + id + "' delivery mode requires field 'size'");
        }
        result.delivery_size = delivery.at("size").get<std::size_t>();
    } else if (delivery.contains("size")) {
        throw std::invalid_argument(
            "Channel '" + id + "' delivery mode must not specify field 'size'");
    }
    if (result.delivery_size == 0) {
        throw std::invalid_argument("Delivery size must be greater than zero for channel '" + id + "'");
    }
    return result;
}

}  // namespace

ChannelManager::ChannelManager(
    ProcessorRegistry& registry,
    ProcessorScheduler& scheduler,
    ZmqTransport& transport,
    ProcessorContext context,
    MetricsRegistry* metrics)
    : registry_(registry),
      scheduler_(scheduler),
      transport_(transport),
      context_(context),
      metrics_(metrics) {}

void ChannelManager::configure(const nlohmann::json& channels_config) {
    if (!channels_config.is_object()) {
        throw std::invalid_argument("data-channels must be an object");
    }

    for (const auto& [channel_id, channel_config] : channels_config.items()) {
        if (!channel_config.value("enabled", true)) {
            continue;
        }

        auto channel = std::make_unique<Channel>(
            parse_channel_config(channel_id, channel_config),
            metrics_ ? metrics_->component("channel/" + channel_id) : nullptr);
        auto* channel_ptr = channel.get();
        const auto [_, inserted] = channels_.emplace(channel_id, std::move(channel));
        if (!inserted) {
            throw std::runtime_error("Duplicate channel id: " + channel_id);
        }

        if (!channel_config.contains("processors")) {
            throw std::invalid_argument("Channel '" + channel_id + "' requires array field 'processors'");
        }
        const auto& processors = channel_config.at("processors");
        if (!processors.is_array()) {
            throw std::invalid_argument("processors must be an array in channel '" + channel_id + "'");
        }

        std::unordered_set<std::string> processor_names;
        std::size_t processor_index = 0;
        for (const auto& processor_config : processors) {
            try {
                const auto location = "data-channels." + channel_id + ".processors[" +
                    std::to_string(processor_index) + "]";
                auto definition = ProcessorDefinition::from_json(processor_config, location);
                const auto type = definition.type();
                auto instance_name = definition.name().empty()
                    ? type + "-" + std::to_string(processor_index)
                    : definition.name();
                if (!processor_names.emplace(instance_name).second) {
                    throw std::invalid_argument(
                        "Duplicate processor name '" + instance_name + "'");
                }
                ++processor_index;
                const ConfigurationView plugin_config(
                    definition.configuration(), location + ".config");
                auto processor = registry_.create(
                    type, plugin_config, context_);
                auto capabilities = registry_.capabilities(type);
                auto processor_name = channel_id + "/" + instance_name;

                scheduler_.add_processor(
                    std::move(processor_name),
                    std::move(processor),
                    definition.schedule(),
                    capabilities,
                    [this, channel_ptr, channel_id](std::vector<OutputMessage> output) {
                        auto messages = channel_ptr->accept(std::move(output));
                        for (auto& message : messages) {
                            if (transport_.publish(std::move(message)) != PublishResult::queued) {
                                spdlog::warn(
                                    "Output was not queued for channel '{}'",
                                    channel_id);
                            }
                        }
                    });
            } catch (const std::exception& error) {
                throw std::runtime_error(
                    "Failed to configure processor in channel '" + channel_id +
                    "': " + error.what());
            }
        }
    }
}

}  // namespace zmq_publisher
