#include "zmq_publisher/processor/processor_definition.h"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <nlohmann/json.hpp>
#include <stdexcept>

TEST_CASE("processor definitions separate core fields from plugin configuration") {
    const auto definition = zmq_publisher::ProcessorDefinition::from_json(
        {
            {"type", "custom"},
            {"name", "source"},
            {"schedule", {{"mode", "fixed-rate"}, {"interval-us", 250}}},
            {"config", {{"custom-field", 42}}},
        },
        "processor");

    CHECK(definition.type() == "custom");
    CHECK(definition.name() == "source");
    CHECK(definition.schedule().mode == zmq_publisher::ScheduleMode::fixed_rate);
    CHECK(definition.schedule().interval == std::chrono::microseconds(250));
    CHECK(definition.configuration() == nlohmann::json{{"custom-field", 42}});
}

TEST_CASE("continuous definitions allow an explicit zero idle backoff") {
    const auto definition = zmq_publisher::ProcessorDefinition::from_json(
        {
            {"type", "source"},
            {"schedule", {{"mode", "continuous"}, {"idle-backoff-us", 0}}},
            {"config", nlohmann::json::object()},
        },
        "processor");

    CHECK(definition.schedule().mode == zmq_publisher::ScheduleMode::continuous);
    CHECK(definition.schedule().idle_backoff == std::chrono::microseconds::zero());
}

TEST_CASE("event-driven definitions do not require polling intervals") {
    const auto definition = zmq_publisher::ProcessorDefinition::from_json(
        {
            {"type", "source"},
            {"schedule", {{"mode", "event-driven"}}},
            {"config", nlohmann::json::object()},
        },
        "processor");

    CHECK(definition.schedule().mode == zmq_publisher::ScheduleMode::event_driven);
    CHECK(definition.schedule().interval == std::chrono::microseconds::zero());
}

TEST_CASE("processor definitions reject fields outside the canonical envelope") {
    CHECK_THROWS_AS(
        zmq_publisher::ProcessorDefinition::from_json(
            {
                {"processor", "legacy"},
                {"period-ms", 10},
                {"config", nlohmann::json::object()},
            },
            "processor"),
        std::invalid_argument);
}
