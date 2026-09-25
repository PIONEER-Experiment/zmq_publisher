#pragma once

#include "zmq_publisher/transport/publish_message.h"
#include "zmq_publisher/transport/publish_result.h"
#include "zmq_publisher/transport/transport_config.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace zmq_publisher {

class MetricsRegistry;
class TransportWorker;

class ZmqTransport {
public:
    explicit ZmqTransport(TransportConfig config = {}, MetricsRegistry* metrics = nullptr);
    ~ZmqTransport();

    ZmqTransport(const ZmqTransport&) = delete;
    ZmqTransport& operator=(const ZmqTransport&) = delete;

    void start();
    void stop();
    PublishResult publish(PublishMessage message);

private:
    TransportConfig config_;
    MetricsRegistry* metrics_;
    bool running_ = false;
    std::mutex lifecycle_mutex_;
    std::vector<std::unique_ptr<TransportWorker>> workers_;
};

}  // namespace zmq_publisher
