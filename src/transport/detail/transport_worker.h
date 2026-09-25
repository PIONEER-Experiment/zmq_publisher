#pragma once

#include "zmq_publisher/transport/publish_message.h"
#include "zmq_publisher/transport/publish_result.h"
#include "zmq_publisher/transport/queue_policy.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace zmq_publisher {

class ComponentMetrics;

class TransportWorker {
public:
    TransportWorker(std::size_t id,
                    std::size_t queue_capacity,
                    QueuePolicy queue_policy,
                    std::size_t zmq_io_threads,
                    std::shared_ptr<ComponentMetrics> metrics);
    ~TransportWorker();

    TransportWorker(const TransportWorker&) = delete;
    TransportWorker& operator=(const TransportWorker&) = delete;

    void start();
    void stop();
    PublishResult publish(PublishMessage message);

private:
    void io_loop();

    std::size_t id_;
    std::size_t queue_capacity_;
    QueuePolicy queue_policy_;
    std::size_t zmq_io_threads_;
    std::shared_ptr<ComponentMetrics> metrics_;
    bool running_ = false;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<PublishMessage> queue_;
    std::thread thread_;
};

}  // namespace zmq_publisher
