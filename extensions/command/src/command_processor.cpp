#include "zmq_publisher/extensions/command/command_processor.h"

#include <utility>

namespace zmq_publisher::command {

CommandProcessor::CommandProcessor(const CommandProcessorConfig& config)
    : runner_(config.arguments()) {}

ProcessResult CommandProcessor::process() {
    auto output = runner_.execute();
    if (output.empty()) {
        return ProcessResult::idle();
    }
    std::vector<OutputMessage> messages;
    messages.push_back(OutputMessage::from_text(output));
    return ProcessResult::produced(std::move(messages));
}

}  // namespace zmq_publisher::command
