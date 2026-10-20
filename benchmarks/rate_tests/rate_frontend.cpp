#include "mfe.h"
#include "midas.h"
#include "odbxx.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

const char* frontend_name = "ZmqPublisherRateSource";
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

std::chrono::microseconds polling_interval{1000};
std::chrono::steady_clock::time_point last_poll;
std::vector<std::byte> payload(1024);
std::uint64_t sequence = 0;
std::uint64_t maximum_events = 0;
std::chrono::microseconds flush_interval{1000};
std::chrono::steady_clock::time_point last_flush;

INT configured_write_cache_size() {
    const char* value = std::getenv("ZMQ_RATE_TEST_WRITE_CACHE_BYTES");
    if (value == nullptr) {
        return 0;
    }
    char* end = nullptr;
    const auto parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0 || parsed > INT_MAX) {
        return 0;
    }
    return static_cast<INT>(parsed);
}

INT configured_flush_interval_us() {
    const char* value = std::getenv("ZMQ_RATE_TEST_FLUSH_INTERVAL_US");
    if (value == nullptr) {
        return 1000;
    }
    char* end = nullptr;
    const auto parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < -1 || parsed > INT_MAX) {
        return 1000;
    }
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

std::string settings_path() {
    char path[128];
    std::snprintf(
        path, sizeof(path), "/Equipment/ZMQ Rate Source %02d/Settings", frontend_index);
    return path;
}

}  // namespace

INT frontend_init() {
    frontend_index = get_frontend_index();
    midas::odb settings = {
        {"Payload Bytes", 1024},
        {"Polling Interval (us)", 1000},
        {"Payload Pattern", 90},
        {"Maximum Events", 0},
    };
    settings.connect(settings_path());
    flush_interval = std::chrono::microseconds(configured_flush_interval_us());
    last_flush = std::chrono::steady_clock::now();
    return SUCCESS;
}

INT frontend_exit() { return SUCCESS; }

INT begin_of_run(INT, char*) {
    midas::odb settings;
    settings.connect(settings_path());

    const auto payload_bytes = static_cast<std::size_t>(settings["Payload Bytes"]);
    const auto interval_us = static_cast<int>(settings["Polling Interval (us)"]);
    const auto pattern = static_cast<unsigned char>(settings["Payload Pattern"]);
    maximum_events = static_cast<std::uint64_t>(settings["Maximum Events"]);
    if (payload_bytes == 0 || payload_bytes + sizeof(TimingRecord) + 128U >
            static_cast<std::size_t>(max_event_size)) {
        cm_msg(MERROR, "begin_of_run", "Invalid payload size: %zu", payload_bytes);
        return FE_ERR_ODB;
    }
    if (interval_us < 0) {
        cm_msg(MERROR, "begin_of_run", "Polling interval cannot be negative");
        return FE_ERR_ODB;
    }

    payload.assign(payload_bytes, static_cast<std::byte>(pattern));
    polling_interval = std::chrono::microseconds(interval_us);
    last_poll = std::chrono::steady_clock::now();
    sequence = 0;
    cm_msg(MINFO,
           "begin_of_run",
           "Rate source configured for %zu-byte payloads at %d us intervals",
           payload_bytes,
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
        if (!test) {
            last_poll = now;
        }
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

    TimingRecord timing;
    timing.sequence = ++sequence;
    timing.monotonic_nanoseconds = monotonic_nanoseconds();
    timing.realtime_nanoseconds = realtime_nanoseconds();
    timing.payload_bytes = static_cast<std::uint32_t>(payload.size());

    bk_init32(event);

    void* timing_data = nullptr;
    bk_create(event, "RTIM", TID_BYTE, &timing_data);
    std::memcpy(timing_data, &timing, sizeof(timing));
    bk_close(event, static_cast<char*>(timing_data) + sizeof(timing));

    void* payload_data = nullptr;
    bk_create(event, "DATA", TID_BYTE, &payload_data);
    std::memcpy(payload_data, payload.data(), payload.size());
    bk_close(event, static_cast<char*>(payload_data) + payload.size());

    return bk_size(event);
}

EQUIPMENT equipment[] = {
    {
        "ZMQ Rate Source %02d",
        {
            42,
            0,
            "SYSTEM",
            EQ_POLLED,
            0,
            "MIDAS",
            TRUE,
            RO_RUNNING,
            100,
            0,
            0,
            TRUE,
            "",
            "",
            "",
            "",
            "",
            FALSE,
            configured_write_cache_size(),
        },
        read_trigger_event,
    },
    {""},
};
