#include "detector_payload_generator.h"

#include "mfe.h"
#include "midas.h"
#include "odbxx.h"

#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#if defined(PIONEER_SIMULATOR_SAMPIC)
const char* frontend_name = "PioneerSampicSimulator";
#define PIONEER_EQUIPMENT_NAME "PIONEER SAMPIC Simulator %02d"
#elif defined(PIONEER_SIMULATOR_MUSIP)
const char* frontend_name = "PioneerMusipSimulator";
#define PIONEER_EQUIPMENT_NAME "PIONEER MUSIP Simulator %02d"
#elif defined(PIONEER_SIMULATOR_HDSOC)
const char* frontend_name = "PioneerHdsocSimulator";
#define PIONEER_EQUIPMENT_NAME "PIONEER HDSoC Simulator %02d"
#elif defined(PIONEER_SIMULATOR_WFD5)
const char* frontend_name = "PioneerWfd5Simulator";
#define PIONEER_EQUIPMENT_NAME "PIONEER WFD5 Simulator %02d"
#else
#error "A detector simulator must select SAMPIC, MUSIP, HDSOC, or WFD5"
#endif

const char* frontend_file_name = __FILE__;
BOOL frontend_call_loop = FALSE;
INT display_period = 0;
INT max_event_size = 8 * 1024 * 1024;
INT max_event_size_frag = 5 * max_event_size;
INT event_buffer_size = 8 * max_event_size;
INT frontend_index;
BOOL equipment_common_overwrite = TRUE;
extern EQUIPMENT equipment[];

namespace {

constexpr std::uint32_t kTimingMagic = 0x5254494d;

struct TimingRecord {
    std::uint32_t magic = kTimingMagic;
    std::uint16_t version = 1;
    std::uint16_t reserved = 0;
    std::uint64_t sequence = 0;
    std::uint64_t monotonic_nanoseconds = 0;
    std::uint64_t realtime_nanoseconds = 0;
    std::uint32_t payload_bytes = 0;
    std::uint32_t reserved2 = 0;
};

static_assert(sizeof(TimingRecord) == 40);

std::unique_ptr<zmq_publisher::benchmarks::DetectorPayloadGenerator> generator;
std::vector<zmq_publisher::benchmarks::DetectorBank> banks;
std::chrono::microseconds polling_interval{1000};
std::chrono::steady_clock::time_point last_poll;
std::chrono::microseconds flush_interval{1000};
std::chrono::steady_clock::time_point last_flush;
std::uint64_t sequence = 0;
std::uint64_t maximum_events = 0;
std::size_t requested_payload_bytes = 0;
std::size_t generated_payload_bytes = 0;

INT configured_write_cache_size() {
    const char* value = std::getenv("ZMQ_RATE_TEST_WRITE_CACHE_BYTES");
    if (value == nullptr) return 0;
    char* end = nullptr;
    const auto parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0 || parsed > INT_MAX) return 0;
    return static_cast<INT>(parsed);
}

INT configured_flush_interval_us() {
    const char* value = std::getenv("ZMQ_RATE_TEST_FLUSH_INTERVAL_US");
    if (value == nullptr) return 1000;
    char* end = nullptr;
    const auto parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < -1 || parsed > INT_MAX) return 1000;
    return static_cast<INT>(parsed);
}

std::uint64_t monotonic_nanoseconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

std::uint64_t realtime_nanoseconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string equipment_name() {
    char name[128];
    std::snprintf(name, sizeof(name), PIONEER_EQUIPMENT_NAME, frontend_index);
    return name;
}

std::string settings_path() {
    return "/Equipment/" + equipment_name() + "/Settings";
}

}  // namespace

INT frontend_init() {
    frontend_index = get_frontend_index();
    generator = zmq_publisher::benchmarks::make_detector_payload_generator();
    midas::odb settings = {
        {"Payload Bytes", 1024},
        {"Polling Interval (us)", 1000},
        {"Maximum Events", 0},
    };
    settings.connect(settings_path());
    flush_interval = std::chrono::microseconds(configured_flush_interval_us());
    last_flush = std::chrono::steady_clock::now();
    return SUCCESS;
}

INT frontend_exit() {
    banks.clear();
    generator.reset();
    return SUCCESS;
}

INT begin_of_run(INT, char*) {
    midas::odb settings;
    settings.connect(settings_path());
    requested_payload_bytes = static_cast<std::size_t>(settings["Payload Bytes"]);
    const auto interval_us = static_cast<int>(settings["Polling Interval (us)"]);
    maximum_events = static_cast<std::uint64_t>(settings["Maximum Events"]);
    if (requested_payload_bytes == 0 || requested_payload_bytes + sizeof(TimingRecord) + 512U >
            static_cast<std::size_t>(max_event_size)) {
        cm_msg(MERROR, "begin_of_run", "Invalid payload size: %zu", requested_payload_bytes);
        return FE_ERR_ODB;
    }
    if (interval_us < 0) {
        cm_msg(MERROR, "begin_of_run", "Polling interval cannot be negative");
        return FE_ERR_ODB;
    }
    try {
        banks = generator->generate(requested_payload_bytes, 0);
    } catch (const std::exception& exception) {
        cm_msg(MERROR, "begin_of_run", "Payload generation failed: %s", exception.what());
        return FE_ERR_ODB;
    }
    generated_payload_bytes = 0;
    for (const auto& bank : banks) generated_payload_bytes += bank.bytes.size();
    polling_interval = std::chrono::microseconds(interval_us);
    last_poll = std::chrono::steady_clock::now();
    sequence = 0;
    cm_msg(MINFO,
           "begin_of_run",
           "%s simulator requested %zu bytes, generated %zu bytes at %d us intervals",
           generator->detector_name().data(),
           requested_payload_bytes,
           generated_payload_bytes,
           interval_us);
    return SUCCESS;
}

INT end_of_run(INT, char*) { return SUCCESS; }
INT pause_run(INT, char*) { return SUCCESS; }
INT resume_run(INT, char*) { return SUCCESS; }
INT frontend_loop() { return SUCCESS; }

INT poll_event(INT, INT, BOOL test) {
    if (maximum_events != 0 && sequence >= maximum_events) return FALSE;
    const auto now = std::chrono::steady_clock::now();
    if (polling_interval.count() == 0 || now - last_poll >= polling_interval) {
        if (!test) last_poll = now;
        return TRUE;
    }
    return FALSE;
}

INT interrupt_configure(INT, INT, POINTER_T) { return SUCCESS; }

INT read_trigger_event(char* event, INT) {
    const auto now = std::chrono::steady_clock::now();
    if (flush_interval.count() >= 0 &&
        (flush_interval.count() == 0 || now - last_flush >= flush_interval)) {
        bm_flush_cache(equipment[0].buffer_handle, BM_WAIT);
        last_flush = now;
    }

    const auto event_sequence = ++sequence;
    try {
        banks = generator->generate(requested_payload_bytes, event_sequence);
    } catch (const std::exception& exception) {
        cm_msg(MERROR, "read_trigger_event", "Payload generation failed: %s", exception.what());
        return 0;
    }
    generated_payload_bytes = 0;
    for (const auto& bank : banks) generated_payload_bytes += bank.bytes.size();

    TimingRecord timing;
    timing.sequence = event_sequence;
    timing.monotonic_nanoseconds = monotonic_nanoseconds();
    timing.realtime_nanoseconds = realtime_nanoseconds();
    timing.payload_bytes = static_cast<std::uint32_t>(generated_payload_bytes);

    bk_init32(event);
    void* timing_data = nullptr;
    bk_create(event, "RTIM", TID_BYTE, &timing_data);
    std::memcpy(timing_data, &timing, sizeof(timing));
    bk_close(event, static_cast<char*>(timing_data) + sizeof(timing));

    for (const auto& bank : banks) {
        void* bank_data = nullptr;
        bk_create(event, bank.name.data(), TID_BYTE, &bank_data);
        std::memcpy(bank_data, bank.bytes.data(), bank.bytes.size());
        bk_close(event, static_cast<char*>(bank_data) + bank.bytes.size());
    }
    return bk_size(event);
}

EQUIPMENT equipment[] = {
    {
        PIONEER_EQUIPMENT_NAME,
        {
            42, 0, "SYSTEM", EQ_POLLED, 0, "MIDAS", TRUE, RO_RUNNING,
            100, 0, 0, TRUE, "", "", "", "", "", FALSE,
            configured_write_cache_size(),
        },
        read_trigger_event,
    },
    {""},
};
