#ifndef EVENTLOOPMANAGER_H
#define EVENTLOOPMANAGER_H
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::milliseconds


class EventLoopManager {
public:
    EventLoopManager(int maxEventsPerSleep, int sleepTimeMillis, int timeoutMillis, int verbose = 0);

    void ManageLoop(int success);
    
    void SetVerbose(int verbose);
    void SetSleepTime(int sleepTimeMillis);
    void SetTimeout(int timeoutMillis);
    void SetMaxEventsPerSleep(int maxEventsPerSleep);

    int GetSleepTime() const;
    int GetMaxEventsPerSleep() const;

private:
    int eventsGrabbed;
    int maxEventsPerSleep;
    int sleepTimeMillis;
    int timeoutMillis;
    int verbose;

    bool DoSleep();
    void Update();
};

#endif // EVENTLOOPMANAGER_H
