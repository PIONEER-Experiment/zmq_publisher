#pragma once

#include "MidasReceiver.h"
#include "zmq_publisher/extensions/midas/midas_processor_config.h"
#include "zmq_publisher/processor/processor.h"

#include <chrono>
#include <mutex>

namespace zmq_publisher::midas {

class MidasProcessor final : public Processor {
public:
    explicit MidasProcessor(const MidasProcessorConfig& config);
    ~MidasProcessor() override;

    MidasProcessor(const MidasProcessor&) = delete;
    MidasProcessor& operator=(const MidasProcessor&) = delete;

    ProcessResult process() override;
    bool is_ready() const override;

private:
    static std::mutex instance_mutex_;
    static bool instance_active_;

    MidasReceiver& receiver_;
    std::chrono::system_clock::time_point last_event_timestamp_;
    std::size_t events_per_retrieval_ = 1;
    bool initialized_ = false;
};

}  // namespace zmq_publisher::midas
