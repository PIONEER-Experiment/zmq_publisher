#pragma once

#include "MidasReceiver.h"
#include "zmq_publisher/extensions/midas/midas_processor_config.h"
#include "zmq_publisher/processor/processor.h"

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
    bool has_pending_work() const override;
    bool set_wakeup_handler(WakeupHandler handler) override;

private:
    static std::mutex instance_mutex_;
    static bool instance_active_;

    MidasReceiver& receiver_;
    std::size_t events_per_retrieval_ = 1;
    bool initialized_ = false;
};

}  // namespace zmq_publisher::midas
