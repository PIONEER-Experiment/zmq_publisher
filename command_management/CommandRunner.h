#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <string>
#include <vector>
#include <chrono>

class CommandRunner {
public:
    CommandRunner(const std::string& command);
    CommandRunner(const std::vector<std::string>& commandWithArgs);

    void addArgument(const std::string& arg);
    void setWaitTime(int milliseconds);
    std::string execute();
    bool isReadyForExecution() const;
    std::string getCommand() const;
    int getWaitTime() const;
    
protected:
    std::vector<std::string> commandWithArgs_;
    int waitTime_;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastExecutionTime;
};

#endif
