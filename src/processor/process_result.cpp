#include "zmq_publisher/processor/process_result.h"

#include <stdexcept>
#include <utility>

namespace zmq_publisher {

ProcessResult::ProcessResult(
    ProcessStatus status,
    std::vector<OutputMessage> output,
    std::size_t item_count)
    : status_(status), output_(std::move(output)), item_count_(item_count) {}

ProcessResult ProcessResult::produced(
    std::vector<OutputMessage> output,
    std::size_t item_count) {
    if (output.empty()) {
        throw std::invalid_argument("A produced result must contain output");
    }
    if (item_count == 0) item_count = output.size();
    return ProcessResult(
        ProcessStatus::produced_output, std::move(output), item_count);
}

ProcessResult ProcessResult::processed(std::size_t item_count) {
    if (item_count == 0) {
        throw std::invalid_argument("A processed result must contain work");
    }
    return ProcessResult(ProcessStatus::processed, {}, item_count);
}

ProcessResult ProcessResult::idle() {
    return ProcessResult(ProcessStatus::idle, {});
}

ProcessResult ProcessResult::stopped() {
    return ProcessResult(ProcessStatus::stopped, {});
}

}  // namespace zmq_publisher
