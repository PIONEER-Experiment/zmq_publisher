#include "zmq_publisher/utility/signal_handler.h"

#include <cerrno>
#include <pthread.h>
#include <system_error>

namespace zmq_publisher {

SignalHandler::SignalHandler() {
    sigemptyset(&signals_);
    sigaddset(&signals_, SIGINT);
    sigaddset(&signals_, SIGHUP);
    sigaddset(&signals_, SIGTERM);
    const auto status = pthread_sigmask(SIG_BLOCK, &signals_, &previous_mask_);
    if (status != 0) {
        throw std::system_error(status, std::generic_category(), "Could not block stop signals");
    }
}

SignalHandler::~SignalHandler() {
    pthread_sigmask(SIG_SETMASK, &previous_mask_, nullptr);
}

bool SignalHandler::wait_for(std::chrono::milliseconds timeout) const {
    const timespec duration{
        static_cast<time_t>(timeout.count() / 1000),
        static_cast<long>((timeout.count() % 1000) * 1000000),
    };
    const auto signal = sigtimedwait(&signals_, nullptr, &duration);
    if (signal == SIGINT || signal == SIGHUP || signal == SIGTERM) return true;
    if (signal < 0 && (errno == EAGAIN || errno == EINTR)) return false;
    throw std::system_error(errno, std::generic_category(), "Could not wait for stop signal");
}

}  // namespace zmq_publisher
