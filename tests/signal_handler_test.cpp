#include "zmq_publisher/utility/signal_handler.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <csignal>

using namespace std::chrono_literals;

TEST_CASE("stop signals are consumed without installing competing handlers") {
    zmq_publisher::SignalHandler signals;
    REQUIRE(std::raise(SIGINT) == 0);
    CHECK(signals.wait_for(100ms));
    CHECK_FALSE(signals.wait_for(1ms));
}
