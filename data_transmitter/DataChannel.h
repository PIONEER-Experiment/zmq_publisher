#ifndef DATA_CHANNEL_H
#define DATA_CHANNEL_H

#include <string>

class DataChannel {
public:
    DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak);
    const std::string& getName() const;
    int getEventsBeforeBreak() const;
    int getEventsToIgnoreInBreak() const;
    int getEventsPublished() const;
    int getEventsSeen() const;
    bool isOnBreak() const;
    int getEventsSeenOnBreak() const;
    void published();
    void seen();
    void reset();
    void printAttributes() const;

private:
    std::string name;
    int eventsBeforeBreak;
    int eventsToIgnoreInBreak;
    int eventsPublished;
    int eventsSeen;
    bool onBreak;
    int eventsSeenOnBreak;

    bool shouldTakeBreak();
    void startBreak();
};

#endif
