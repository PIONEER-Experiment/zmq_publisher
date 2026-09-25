#include "zmq_publisher/processor/process_result.h"

#include <stdexcept>
#include <utility>

namespace zmq_publisher {

ProcessResult::ProcessResult(ProcessStatus status, std::vector<OutputMessage> output)
    : status_(status), output_(std::move(output)) {}

ProcessResult ProcessResult::produced(std::vector<OutputMessage> output) {
    if (output.empty()) {
        throw std::invalid_argument("A produced result must contain output");
    }
    return ProcessResult(ProcessStatus::produced_output, std::move(output));
}

ProcessResult ProcessResult::idle() {
    return ProcessResult(ProcessStatus::idle, {});
}

ProcessResult ProcessResult::stopped() {
    return ProcessResult(ProcessStatus::stopped, {});
}

}  // namespace zmq_publisher
