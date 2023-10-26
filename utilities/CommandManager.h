#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <string>
#include <vector>
#include <chrono>

class CommandManager {
public:
    CommandManager(const std::string& command);
    CommandManager(const std::vector<std::string>& commandWithArgs);

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
