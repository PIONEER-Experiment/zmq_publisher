#include "zmq_publisher/channel/channel_manager.h"

#include "zmq_publisher/processor/processor_registry.h"
#include "zmq_publisher/scheduler/processor_schedule.h"
#include "zmq_publisher/scheduler/processor_scheduler.h"
#include "zmq_publisher/transport/zmq_transport.h"

#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace zmq_publisher {

namespace {

ProcessorSchedule parse_schedule(const nlohmann::json& processor_config) {
    ProcessorSchedule schedule;
    auto period = processor_config.value("period-ms", 1000);
    std::string mode;

    if (processor_config.contains("schedule")) {
        const auto& schedule_config = processor_config.at("schedule");
        if (!schedule_config.is_object()) {
            throw std::invalid_argument("Processor schedule must be an object");
        }
        period = schedule_config.value("interval-ms", period);
        mode = schedule_config.value("mode", "fixed-delay");
    } else {
        mode = period == 0 ? "continuous" : "fixed-delay";
    }

    if (period < 0) {
        throw std::invalid_argument("Processor period cannot be negative");
    }
    schedule.interval = std::chrono::milliseconds(period);
    if (mode == "continuous") {
        schedule.mode = ScheduleMode::continuous;
    } else if (mode == "fixed-rate") {
        schedule.mode = ScheduleMode::fixed_rate;
    } else if (mode == "fixed-delay") {
        schedule.mode = ScheduleMode::fixed_delay;
    } else {
        throw std::invalid_argument("Unknown processor schedule mode: " + mode);
    }
    return schedule;
}

ChannelConfig parse_channel_config(const std::string& id, const nlohmann::json& config) {
    ChannelConfig result;
    result.id = id;
    result.topic = config.value("topic", config.value("name", std::string{}));
    result.endpoint = config.value(
        "endpoint", config.value("zmq-address", std::string{"tcp://127.0.0.1:5555"}));

    const auto publishes_per_batch = config.value("publishes-per-batch", 1);
    const auto ignored_after_batch = config.value("publishes-ignored-after-batch", 0);
    const auto buffer_capacity = config.value(
        "buffer-capacity", config.value("num-events-in-circular-buffer", 1));
    if (publishes_per_batch <= 0 || ignored_after_batch < 0 || buffer_capacity <= 0) {
        throw std::invalid_argument("Invalid numeric channel configuration for '" + id + "'");
    }
    result.publishes_per_batch = static_cast<std::size_t>(publishes_per_batch);
    result.publishes_ignored_after_batch = static_cast<std::size_t>(ignored_after_batch);
    result.buffer_capacity = static_cast<std::size_t>(buffer_capacity);
    return result;
}

}  // namespace

ChannelManager::ChannelManager(
    ProcessorRegistry& registry,
    ProcessorScheduler& scheduler,
    ZmqTransport& transport,
    ProcessorContext context)
    : registry_(registry),
      scheduler_(scheduler),
      transport_(transport),
      context_(context) {}

void ChannelManager::configure(const nlohmann::json& channels_config) {
    if (!channels_config.is_object()) {
        throw std::invalid_argument("data-channels must be an object");
    }

    for (const auto& [channel_id, channel_config] : channels_config.items()) {
        if (!channel_config.value("enabled", true)) {
            continue;
        }

        auto channel = std::make_unique<Channel>(
            parse_channel_config(channel_id, channel_config));
        auto* channel_ptr = channel.get();
        const auto [_, inserted] = channels_.emplace(channel_id, std::move(channel));
        if (!inserted) {
            throw std::runtime_error("Duplicate channel id: " + channel_id);
        }

        if (!channel_config.contains("processors")) {
            spdlog::warn("Channel '{}' has no processors", channel_id);
            continue;
        }
        const auto& processors = channel_config.at("processors");
        if (!processors.is_array()) {
            throw std::invalid_argument("processors must be an array in channel '" + channel_id + "'");
        }

        std::size_t processor_index = 0;
        for (const auto& processor_config : processors) {
            try {
                const auto type = processor_config.value(
                    "type", processor_config.value("processor", std::string{}));
                if (type.empty()) {
                    throw std::invalid_argument("Processor type is required");
                }
                auto processor = registry_.create(type, processor_config, context_);
                auto capabilities = registry_.capabilities(type);
                auto schedule = parse_schedule(processor_config);
                auto processor_name = channel_id + "/" + type + "/" +
                    std::to_string(processor_index++);

                scheduler_.add_processor(
                    std::move(processor_name),
                    std::move(processor),
                    schedule,
                    capabilities,
                    [this, channel_ptr, channel_id](std::vector<std::string> output) {
                        auto message = channel_ptr->accept(std::move(output));
                        if (message && !transport_.publish(std::move(*message))) {
                            spdlog::warn(
                                "Dropping output for channel '{}': transport queue is full or stopped",
                                channel_id);
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
