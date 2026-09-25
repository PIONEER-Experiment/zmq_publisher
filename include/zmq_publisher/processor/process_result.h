#pragma once

#include "zmq_publisher/processor/output_message.h"

#include <vector>

namespace zmq_publisher {

enum class ProcessStatus {
    produced_output,
    idle,
    stopped,
};

class ProcessResult {
public:
    static ProcessResult produced(std::vector<OutputMessage> output);
    static ProcessResult idle();
    static ProcessResult stopped();

    ProcessStatus status() const noexcept { return status_; }
    std::vector<OutputMessage>& output() noexcept { return output_; }
    const std::vector<OutputMessage>& output() const noexcept { return output_; }

private:
    ProcessResult(ProcessStatus status, std::vector<OutputMessage> output);

    ProcessStatus status_;
    std::vector<OutputMessage> output_;
};

}  // namespace zmq_publisher
