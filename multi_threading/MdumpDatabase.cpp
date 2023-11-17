#include "MdumpDatabase.h"
#include <algorithm>
#include <utility>
#include "ProjectPrinter.h"

MdumpDatabase::MdumpDatabase(const CommandRunner& commandRunner, int verbose_level, int maxWorkers)
    : commandRunner(commandRunner), maxWorkers(maxWorkers), newData(false), verbose(verbose_level){}

MdumpDatabase::~MdumpDatabase() {
    stopWorkers();
}

void MdumpDatabase::startWorker() {
    ProjectPrinter printer;
    joinFinishedThreads();

    if (runningWorkers.size() < maxWorkers) {
        if (verbose > 1) {
            printer.Print("Launched mdump worker, currently there are " + std::to_string(runningWorkers.size()) + "/"  + std::to_string(maxWorkers) + " workers");
        }
        std::lock_guard<std::mutex> lock(workerMutex);
        std::thread worker(&MdumpDatabase::updateMdumpOutput, this);
        std::thread::id workerId = worker.get_id();
        runningWorkers[workerId] = std::move(worker);
    } else {
        if (verbose > 1) {
           printer.Print("Did not launch worker, currently there are " + std::to_string(runningWorkers.size()) + "/"  + std::to_string(maxWorkers) + " workers");
        }
        
    }
}

std::string MdumpDatabase::getLatestMdumpOutput() {
    std::lock_guard<std::mutex> lock(databaseMutex);
    newData = false;
    return latestOutput;
}

bool MdumpDatabase::hasNewData() const {
    return newData;
}

void MdumpDatabase::updateMdumpOutput() {
    ProjectPrinter printer;

    // Run commandRunner.execute() to get the latest MdumpPackage
    std::string newOutput = commandRunner.execute();

    // Update the member variables in a synchronized manner
    {
        std::lock_guard<std::mutex> lock(databaseMutex);
        latestOutput = newOutput;
        newData = true;
    }

    {
        std::lock_guard<std::mutex> lock(joinMutex);
        threadsToJoin.push_back(std::this_thread::get_id());
    } 
}


void MdumpDatabase::stopWorkers() {
    for (auto& pair : runningWorkers) {
        if (pair.second.joinable()) {
            pair.second.join();  // Wait for the worker to finish
        }
    }
}


void MdumpDatabase::joinFinishedThreads() {
    std::unique_lock<std::mutex> lock(joinMutex);

    for (const auto& threadId : threadsToJoin) {
        auto it = runningWorkers.find(threadId);
        if (it != runningWorkers.end() && it->second.joinable()) {
            it->second.join();  // Join the thread
            runningWorkers.erase(it);  // Erase the joined thread from the map
        }
    }

    threadsToJoin.clear();  // Clear the list after joining all threads
}




void MdumpDatabase::setMaxWorkers(int maxWorkers) {
    this->maxWorkers = maxWorkers;
}

void MdumpDatabase::setCommandRunner(const CommandRunner& commandRunner) {
    this->commandRunner = commandRunner;
}
