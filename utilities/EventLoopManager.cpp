#include "EventLoopManager.h"
#include "ProjectPrinter.h"

EventLoopManager::EventLoopManager(int maxEventsPerSleep, int sleepTimeMillis, int timeoutMillis, int verbose)
    : maxEventsPerSleep(maxEventsPerSleep), sleepTimeMillis(sleepTimeMillis), timeoutMillis(timeoutMillis),verbose(verbose), eventsGrabbed(0) {}


void EventLoopManager::ManageLoop(int success) {
    if (verbose > 1 && success != 1) {
        ProjectPrinter printer;
        printer.Print("Timed out (" + std::to_string(timeoutMillis) + " ms)");
    }
    Update();
    if (DoSleep()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(GetSleepTime()));
    }
}

bool EventLoopManager::DoSleep() {
    bool doSleep = maxEventsPerSleep > 0 && (eventsGrabbed % maxEventsPerSleep == 0);
    if (doSleep && (verbose > 0)) {
        ProjectPrinter printer;
        printer.Print("Sleeping for " + std::to_string(sleepTimeMillis) + " milliseconds");
    }
    return doSleep;
}

void EventLoopManager::Update() {
    eventsGrabbed += 1;
}

void EventLoopManager::SetVerbose(int verbose) {
    this->verbose = verbose;
}

void EventLoopManager::SetSleepTime(int sleepTimeMillis) {
    this->sleepTimeMillis = sleepTimeMillis;
}

void EventLoopManager::SetTimeout(int timeoutMillis) {
    this->timeoutMillis = timeoutMillis;
}

void EventLoopManager::SetMaxEventsPerSleep(int maxEventsPerSleep) {
    this->maxEventsPerSleep = maxEventsPerSleep;
}

int EventLoopManager::GetSleepTime() const {
    return sleepTimeMillis;
}

int EventLoopManager::GetMaxEventsPerSleep() const {
    return maxEventsPerSleep;
}
