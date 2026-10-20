#pragma once

#include <chrono>
#include <signal.h>

namespace zmq_publisher {

class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler();

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    bool wait_for(std::chrono::milliseconds timeout) const;

private:
    sigset_t signals_{};
    sigset_t previous_mask_{};
};

}  // namespace zmq_publisher
