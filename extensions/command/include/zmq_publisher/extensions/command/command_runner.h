#pragma once

#include <string>
#include <vector>

namespace zmq_publisher::command {

class CommandRunner {
public:
    explicit CommandRunner(std::vector<std::string> arguments);

    std::string execute() const;

private:
    std::vector<std::string> arguments_;
};

}  // namespace zmq_publisher::command
