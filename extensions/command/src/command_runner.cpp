#include "zmq_publisher/extensions/command/command_runner.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

extern char** environ;

namespace zmq_publisher::command {

CommandRunner::CommandRunner(std::vector<std::string> arguments)
    : arguments_(std::move(arguments)) {
    if (arguments_.empty() || arguments_.front().empty()) {
        throw std::invalid_argument("Command arguments cannot be empty");
    }
}

std::string CommandRunner::execute() const {
    int output_pipe[2];
    if (pipe(output_pipe) != 0) {
        throw std::runtime_error("Failed to create command output pipe: " +
                                 std::string(std::strerror(errno)));
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, output_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, output_pipe[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, output_pipe[0]);
    posix_spawn_file_actions_addclose(&actions, output_pipe[1]);

    std::vector<char*> arguments;
    arguments.reserve(arguments_.size() + 1);
    for (const auto& argument : arguments_) {
        arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    arguments.push_back(nullptr);

    pid_t child = 0;
    const int spawn_result = posix_spawnp(
        &child,
        arguments.front(),
        &actions,
        nullptr,
        arguments.data(),
        environ);
    posix_spawn_file_actions_destroy(&actions);
    close(output_pipe[1]);
    if (spawn_result != 0) {
        close(output_pipe[0]);
        throw std::runtime_error("Failed to start command: " +
                                 std::string(std::strerror(spawn_result)));
    }

    std::string output;
    std::array<char, 4096> buffer{};
    while (true) {
        const auto bytes_read = read(output_pipe[0], buffer.data(), buffer.size());
        if (bytes_read > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(bytes_read));
        } else if (bytes_read == 0) {
            break;
        } else if (errno != EINTR) {
            close(output_pipe[0]);
            waitpid(child, nullptr, 0);
            throw std::runtime_error("Failed to read command output: " +
                                     std::string(std::strerror(errno)));
        }
    }
    close(output_pipe[0]);

    int status = 0;
    if (waitpid(child, &status, 0) < 0) {
        throw std::runtime_error("Failed to wait for command: " +
                                 std::string(std::strerror(errno)));
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Command exited unsuccessfully: " + output);
    }
    return output;
}

}  // namespace zmq_publisher::command
