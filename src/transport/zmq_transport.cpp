#include "zmq_publisher/transport/zmq_transport.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <zmq.hpp>

namespace zmq_publisher {

ZmqTransport::ZmqTransport(std::size_t queue_capacity)
    : queue_capacity_(queue_capacity) {
    if (queue_capacity == 0) {
        throw std::invalid_argument("ZMQ output queue capacity must be greater than zero");
    }
}

ZmqTransport::~ZmqTransport() {
    stop();
}

void ZmqTransport::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    io_thread_ = std::thread(&ZmqTransport::io_loop, this);
}

void ZmqTransport::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!running_ && !io_thread_.joinable()) {
            return;
        }
        running_ = false;
    }
    queue_cv_.notify_all();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool ZmqTransport::publish(PublishMessage message) {
    {
        std::lock_guard lock(mutex_);
        if (!running_ || queue_.size() >= queue_capacity_) {
            return false;
        }
        queue_.push_back(std::move(message));
    }
    queue_cv_.notify_one();
    return true;
}

void ZmqTransport::io_loop() {
    zmq::context_t context(1);
    std::unordered_map<std::string, std::unique_ptr<zmq::socket_t>> sockets;

    while (true) {
        PublishMessage message;
        {
            std::unique_lock lock(mutex_);
            queue_cv_.wait(lock, [this] { return !running_ || !queue_.empty(); });
            if (queue_.empty()) {
                if (!running_) {
                    break;
                }
                continue;
            }
            message = std::move(queue_.front());
            queue_.pop_front();
        }

        try {
            auto found = sockets.find(message.endpoint);
            if (found == sockets.end()) {
                auto socket = std::make_unique<zmq::socket_t>(context, zmq::socket_type::pub);
                socket->bind(message.endpoint);
                found = sockets.emplace(message.endpoint, std::move(socket)).first;
                spdlog::info("Bound ZMQ publisher to {}", message.endpoint);
            }

            if (!message.topic.empty()) {
                found->second->send(zmq::buffer(message.topic), zmq::send_flags::sndmore);
            }
            found->second->send(zmq::buffer(message.payload), zmq::send_flags::none);
        } catch (const zmq::error_t& error) {
            spdlog::error("Failed to publish to '{}': {}", message.endpoint, error.what());
            sockets.erase(message.endpoint);
        }
    }

    sockets.clear();
    context.shutdown();
    context.close();
}

}  // namespace zmq_publisher
