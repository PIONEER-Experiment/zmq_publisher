#include "rate_subscriber.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>

namespace {
std::atomic<bool> running{true};

void stop(int) {
    running.store(false, std::memory_order_relaxed);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " ENDPOINT OUTPUT.csv\n";
        return 2;
    }
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    try {
        zmq_publisher::benchmarks::RateSubscriber subscriber(argv[1], argv[2]);
        subscriber.run(running);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
