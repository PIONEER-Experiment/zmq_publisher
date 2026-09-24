#include "zmq_publisher/processor/processor_registry.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class FakeProcessor final : public zmq_publisher::Processor {
public:
    explicit FakeProcessor(std::string value) : value_(std::move(value)) {}
    std::vector<std::string> process() override { return {value_}; }

private:
    std::string value_;
};

}  // namespace

TEST_CASE("the registry creates processors registered by an extension") {
    zmq_publisher::ProcessorRegistry registry;
    registry.register_processor("fake", {
        [](const nlohmann::json& config, zmq_publisher::ProcessorContext&) {
            return std::make_unique<FakeProcessor>(config.at("value").get<std::string>());
        },
        {},
    });

    CHECK(registry.contains("fake"));
    zmq_publisher::ProcessorContext context;
    auto processor = registry.create("fake", {{"value", "configured-by-extension"}}, context);
    CHECK(processor->process().front() == "configured-by-extension");
}

TEST_CASE("the registry rejects an unknown processor type") {
    zmq_publisher::ProcessorRegistry registry;
    zmq_publisher::ProcessorContext context;
    CHECK_THROWS_AS(
        registry.create("missing", nlohmann::json::object(), context),
        std::runtime_error);
}
