#include "zmq_publisher/extension/plugin_loader.h"
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
    auto processor = registry.create(
        "command",
        {{"config", {{"command", {"/bin/printf", "extension-loaded"}}}}},
        context);
    const auto output = processor->process();
    REQUIRE(output.size() == 1);
    CHECK(output.front() == "extension-loaded");
}
