#include "zmq_publisher/processor/processor_definition.h"

#include "zmq_publisher/config/configuration_view.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace zmq_publisher {

namespace {

std::chrono::microseconds parse_interval(
    const ConfigurationView& schedule) {
    const bool has_microseconds = schedule.contains("interval-us");
    const bool has_milliseconds = schedule.contains("interval-ms");
    if (has_microseconds == has_milliseconds) {
        throw std::invalid_argument(
            schedule.location() +
            " must contain exactly one of 'interval-us' or 'interval-ms'");
    }

    const auto interval = has_microseconds
        ? std::chrono::microseconds(schedule.required<long long>("interval-us"))
        : std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::milliseconds(schedule.required<long long>("interval-ms")));
    if (interval.count() <= 0) {
        throw std::invalid_argument(
            schedule.location() + " interval must be greater than zero");
    }
    return interval;
}

ProcessorSchedule parse_schedule(const ConfigurationView& value) {
    value.validate_keys({"mode", "interval-us", "interval-ms", "idle-backoff-us"});

    ProcessorSchedule result;
    const auto mode = value.required<std::string>("mode");
    if (mode == "continuous") {
        if (value.contains("interval-us") || value.contains("interval-ms")) {
            throw std::invalid_argument(
                value.location() + " must not specify an interval in continuous mode");
        }
        result.mode = ScheduleMode::continuous;
        result.interval = std::chrono::microseconds::zero();
        result.idle_backoff = std::chrono::microseconds(
            value.optional<long long>("idle-backoff-us", 10'000LL));
        if (result.idle_backoff.count() < 0) {
            throw std::invalid_argument(
                value.location() + " idle backoff cannot be negative");
        }
        return result;
    }

    if (value.contains("idle-backoff-us")) {
        throw std::invalid_argument(
            value.location() +
            " may only specify 'idle-backoff-us' in continuous mode");
    }
    result.interval = parse_interval(value);
    if (mode == "fixed-delay") {
        result.mode = ScheduleMode::fixed_delay;
    } else if (mode == "fixed-rate") {
        result.mode = ScheduleMode::fixed_rate;
    } else {
        throw std::invalid_argument(
            value.location() + " has unknown mode '" + mode + "'");
    }
    return result;
}

}  // namespace

ProcessorDefinition::ProcessorDefinition(
    std::string type,
    std::string name,
    ProcessorSchedule schedule,
    nlohmann::json configuration)
    : type_(std::move(type)),
      name_(std::move(name)),
      schedule_(schedule),
      configuration_(std::move(configuration)) {}

ProcessorDefinition ProcessorDefinition::from_json(
    const nlohmann::json& value,
    std::string location) {
    const ConfigurationView definition(value, std::move(location));
    definition.validate_keys({"type", "name", "schedule", "config"});

    auto type = definition.required<std::string>("type");
    if (type.empty()) {
        throw std::invalid_argument(
            definition.location() + " field 'type' cannot be empty");
    }

    auto name = definition.optional<std::string>("name", {});
    if (definition.contains("name")) {
        if (name.empty()) {
            throw std::invalid_argument(
                definition.location() + " field 'name' cannot be empty");
        }
    }

    const auto schedule = definition.object("schedule");
    const auto configuration = definition.object("config");

    return ProcessorDefinition(
        std::move(type),
        std::move(name),
        parse_schedule(schedule),
        configuration.value());
}

}  // namespace zmq_publisher
