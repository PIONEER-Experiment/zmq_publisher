#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace zmq {
class context_t;
}

namespace zmq_publisher::benchmarks {

class RateSubscriber {
public:
    RateSubscriber(std::string endpoint, std::string output_path);

    void run(const std::atomic<bool>& running);

private:
    struct SourceTiming {
        std::uint64_t sequence;
        std::uint64_t monotonic_ns;
        std::uint32_t payload_bytes;
    };

    static SourceTiming parse_source_timing(const std::byte* data, std::size_t size);
    void write_header();

    std::string endpoint_;
    std::ofstream output_;
};

}  // namespace zmq_publisher::benchmarks
