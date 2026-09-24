#include "zmq_publisher/extensions/command/command_processor.h"

#include <utility>

namespace zmq_publisher::command {

CommandProcessor::CommandProcessor(std::vector<std::string> arguments)
    : runner_(std::move(arguments)) {}

std::vector<std::string> CommandProcessor::process() {
    auto output = runner_.execute();
    if (output.empty()) {
        return {};
    }
    return {std::move(output)};
}

}  // namespace zmq_publisher::command
