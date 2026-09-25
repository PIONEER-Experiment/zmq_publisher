#pragma once

#include <csignal>

namespace zmq_publisher {

class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler();

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    bool stop_requested() const noexcept;

private:
    static void handle_signal(int signal) noexcept;
    static volatile std::sig_atomic_t stop_requested_;
};

}  // namespace zmq_publisher
