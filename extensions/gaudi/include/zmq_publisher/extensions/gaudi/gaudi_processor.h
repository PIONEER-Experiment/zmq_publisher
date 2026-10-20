#pragma once

#include "MidasReceiver.h"
#include "zmq_publisher/extensions/gaudi/gaudi_client.h"
#include "zmq_publisher/extensions/gaudi/gaudi_processor_config.h"
#include "zmq_publisher/extensions/gaudi/sidecar_process.h"
#include "zmq_publisher/processor/processor.h"

#include <mutex>
#include <chrono>

namespace zmq_publisher::gaudi {

class GaudiProcessor final : public Processor {
public:
    explicit GaudiProcessor(const GaudiProcessorConfig& config);
    ~GaudiProcessor() override;

    ProcessResult process() override;
    bool is_ready() const override;
    bool has_pending_work() const override;
    bool set_wakeup_handler(WakeupHandler handler) override;

private:
    static std::mutex instance_mutex_;
    static bool instance_active_;

    MidasReceiver& receiver_;
    SidecarProcess sidecar_;
    GaudiClient client_;
    std::size_t events_per_request_;
    std::size_t maximum_batch_bytes_;
    std::uint64_t publish_every_events_;
    std::chrono::milliseconds publish_every_;
    std::uint64_t events_since_publication_ = 0;
    std::chrono::steady_clock::time_point last_publication_ =
        std::chrono::steady_clock::now();
    bool initialized_ = false;
};

}  // namespace zmq_publisher::gaudi
