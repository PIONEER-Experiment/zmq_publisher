#include "zmq_publisher/extensions/gaudi/sidecar_process.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>

#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>

namespace zmq_publisher::gaudi {
namespace {

std::runtime_error process_error(const std::string& action) {
    return std::runtime_error(action + ": " + std::strerror(errno));
}

}  // namespace

SidecarProcess::SidecarProcess(
    const std::string& launcher,
    const std::string& job_options,
    const std::string& socket_path,
    std::chrono::milliseconds startup_timeout) {
    if (launcher.empty()) return;

    std::error_code removal_error;
    std::filesystem::remove(socket_path, removal_error);
    if (removal_error) {
        throw std::runtime_error(
            "Could not remove stale Gaudi socket: " + removal_error.message());
    }

    process_id_ = ::fork();
    if (process_id_ < 0) throw process_error("Could not fork Gaudi sidecar");
    if (process_id_ == 0) {
        sigset_t empty_mask;
        ::sigemptyset(&empty_mask);
        ::sigprocmask(SIG_SETMASK, &empty_mask, nullptr);
        ::setpgid(0, 0);
        // Ensure an abnormal publisher exit cannot leave the containerized
        // Gaudi service running without an owner.
        if (::prctl(PR_SET_PDEATHSIG, SIGTERM) != 0 || ::getppid() == 1) {
            ::_exit(126);
        }
        if (::setenv("PIONEER_GAUDI_SOCKET", socket_path.c_str(), 1) != 0) {
            ::_exit(126);
        }
        ::execl(
            launcher.c_str(), launcher.c_str(), "--job-options", job_options.c_str(),
            static_cast<char*>(nullptr));
        ::_exit(127);
    }
    ::setpgid(process_id_, process_id_);

    const auto deadline = std::chrono::steady_clock::now() + startup_timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        int status = 0;
        const auto result = ::waitpid(process_id_, &status, WNOHANG);
        if (result == process_id_) {
            process_id_ = -1;
            throw std::runtime_error("Gaudi sidecar exited before opening its socket");
        }
        if (result < 0) {
            process_id_ = -1;
            throw process_error("Could not inspect Gaudi sidecar process");
        }
        if (std::filesystem::exists(socket_path)) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ::kill(-process_id_, SIGTERM);
    ::waitpid(process_id_, nullptr, 0);
    process_id_ = -1;
    throw std::runtime_error("Gaudi sidecar did not open its socket before startup timeout");
}

SidecarProcess::~SidecarProcess() {
    if (process_id_ < 0) return;
    ::kill(-process_id_, SIGTERM);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        const auto result = ::waitpid(process_id_, nullptr, WNOHANG);
        if (result == process_id_ || (result < 0 && errno == ECHILD)) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ::kill(-process_id_, SIGKILL);
    ::waitpid(process_id_, nullptr, 0);
}

}  // namespace zmq_publisher::gaudi
