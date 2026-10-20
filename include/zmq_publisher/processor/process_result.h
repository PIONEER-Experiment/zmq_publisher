#pragma once

#include "zmq_publisher/processor/output_message.h"

#include <vector>

namespace zmq_publisher {

enum class ProcessStatus {
    produced_output,
    processed,
    idle,
    stopped,
};

class ProcessResult {
public:
    static ProcessResult produced(
        std::vector<OutputMessage> output,
        std::size_t item_count = 0);
    static ProcessResult processed(std::size_t item_count);
    static ProcessResult idle();
    static ProcessResult stopped();

    ProcessStatus status() const noexcept { return status_; }
    std::vector<OutputMessage>& output() noexcept { return output_; }
    const std::vector<OutputMessage>& output() const noexcept { return output_; }
    std::size_t item_count() const noexcept { return item_count_; }

private:
    ProcessResult(
        ProcessStatus status,
        std::vector<OutputMessage> output,
        std::size_t item_count = 0);

    ProcessStatus status_;
    std::vector<OutputMessage> output_;
    std::size_t item_count_ = 0;
};

}  // namespace zmq_publisher
