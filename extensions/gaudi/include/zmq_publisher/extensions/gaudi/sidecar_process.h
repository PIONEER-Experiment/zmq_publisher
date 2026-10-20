#pragma once

#include <chrono>
#include <string>

#include <sys/types.h>

namespace zmq_publisher::gaudi {

class SidecarProcess {
public:
    SidecarProcess(
        const std::string& launcher,
        const std::string& job_options,
        const std::string& socket_path,
        std::chrono::milliseconds startup_timeout);
    ~SidecarProcess();

    SidecarProcess(const SidecarProcess&) = delete;
    SidecarProcess& operator=(const SidecarProcess&) = delete;

private:
    pid_t process_id_ = -1;
};

}  // namespace zmq_publisher::gaudi
