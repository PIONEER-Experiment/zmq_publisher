#include "zmq_publisher/utility/signal_handler.h"

namespace zmq_publisher {

volatile std::sig_atomic_t SignalHandler::stop_requested_ = 0;

SignalHandler::SignalHandler() {
    stop_requested_ = 0;
    std::signal(SIGINT, handle_signal);
    std::signal(SIGHUP, handle_signal);
    std::signal(SIGTERM, handle_signal);
}

SignalHandler::~SignalHandler() {
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGHUP, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
}

bool SignalHandler::stop_requested() const noexcept {
    return stop_requested_ != 0;
}

void SignalHandler::handle_signal(int) noexcept {
    stop_requested_ = 1;
}

}  // namespace zmq_publisher
