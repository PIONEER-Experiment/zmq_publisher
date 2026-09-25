#include "zmq_publisher/processor/processor_registry.h"

#include "zmq_publisher/config/configuration_view.h"

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
    zmq_publisher::ProcessResult process() override {
        std::vector<zmq_publisher::OutputMessage> output;
        output.push_back(zmq_publisher::OutputMessage::from_text(value_));
        return zmq_publisher::ProcessResult::produced(std::move(output));
    }

private:
    std::string value_;
};

}  // namespace

TEST_CASE("the registry creates processors registered by an extension") {
    zmq_publisher::ProcessorRegistry registry;
    registry.register_processor("fake", {
        [](const zmq_publisher::ConfigurationView& config,
           zmq_publisher::ProcessorContext&) {
            config.validate_keys({"value"});
            return std::make_unique<FakeProcessor>(config.required<std::string>("value"));
        },
        {},
    });

    CHECK(registry.contains("fake"));
    zmq_publisher::ProcessorContext context;
    const nlohmann::json config{{"value", "configured-by-extension"}};
    const zmq_publisher::ConfigurationView config_view(config, "test.config");
    auto processor = registry.create("fake", config_view, context);
    const auto result = processor->process();
    REQUIRE(result.output().size() == 1);
    const auto& message = result.output().front();
    CHECK(std::string(
              reinterpret_cast<const char*>(message.payload()->data()), message.size()) ==
          "configured-by-extension");
}

TEST_CASE("the registry rejects an unknown processor type") {
    zmq_publisher::ProcessorRegistry registry;
    zmq_publisher::ProcessorContext context;
    const auto config = nlohmann::json::object();
    const zmq_publisher::ConfigurationView config_view(config, "test.config");
    CHECK_THROWS_AS(
        registry.create("missing", config_view, context),
        std::runtime_error);
}
