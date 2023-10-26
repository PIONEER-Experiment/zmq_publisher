#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <string>
#include <vector>
#include <chrono>

class CommandManager {
public:
    //CommandManager() : waitTime_(0) {}
    CommandManager(const std::string& command);
    CommandManager(const std::vector<std::string>& commandWithArgs);

    void addArgument(const std::string& arg);
    void setWaitTime(int milliseconds);
    std::string execute();
    bool isReadyForExecution(std::chrono::milliseconds time) const;
    
protected:
    std::vector<std::string> commandWithArgs_;
    int waitTime_;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastExecutionTime;
};

#endif
