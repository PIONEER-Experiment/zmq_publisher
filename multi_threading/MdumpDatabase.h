#ifndef MDUMPDATABASE_H
#define MDUMPDATABASE_H

#include "CommandRunner.h"
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include <map>

class MdumpDatabase {
public:
    MdumpDatabase(const CommandRunner& runner = CommandRunner(""), int verbose_level = 0, int maxWorkers = 5);
    MdumpDatabase();
    ~MdumpDatabase();

    void startWorker();
    std::string getLatestMdumpOutput();
    bool hasNewData() const;

    void setMaxWorkers(int maxWorkers);
    void setCommandRunner(const CommandRunner& commandRunner);
    void stopWorkers();
    void joinFinishedThreads();

private:
    void updateMdumpOutput();

    CommandRunner commandRunner;
    std::string latestOutput;
    std::mutex databaseMutex;
    std::mutex workerMutex;
    std::mutex joinMutex;
    std::map<std::thread::id, std::thread> runningWorkers;
    std::vector<std::thread::id> threadsToJoin;
    bool newData;
    int maxWorkers;
    int verbose;
};

#endif // MDUMPDATABASE_H
