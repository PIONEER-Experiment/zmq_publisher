#include "zmq_publisher/extension/plugin_loader.h"
#include "zmq_publisher/config/configuration_view.h"
#include "zmq_publisher/processor/processor_context.h"
#include "zmq_publisher/processor/processor_registry.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <string>

TEST_CASE("the command extension registers and creates its processor") {
    zmq_publisher::ProcessorRegistry registry;
    zmq_publisher::PluginLoader loader(registry);
    loader.load(ZMQ_PUBLISHER_COMMAND_EXTENSION_PATH);
    REQUIRE(registry.contains("command"));

    zmq_publisher::ProcessorContext context;
    const nlohmann::json config{
        {"command", {"/bin/printf", "extension-loaded"}}};
    const zmq_publisher::ConfigurationView config_view(config, "test.config");
    auto processor = registry.create("command", config_view, context);
    const auto output = processor->process();
    REQUIRE(output.output().size() == 1);
    const auto& message = output.output().front();
    CHECK(std::string(
              reinterpret_cast<const char*>(message.payload().data()), message.size()) ==
          "extension-loaded");
}
