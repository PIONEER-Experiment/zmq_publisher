#pragma once

#include "zmq_publisher/extensions/command/command_runner.h"
#include "zmq_publisher/extensions/command/command_processor_config.h"
#include "zmq_publisher/processor/processor.h"

namespace zmq_publisher::command {

class CommandProcessor final : public Processor {
public:
    explicit CommandProcessor(const CommandProcessorConfig& config);

    ProcessResult process() override;

private:
    CommandRunner runner_;
};

}  // namespace zmq_publisher::command
