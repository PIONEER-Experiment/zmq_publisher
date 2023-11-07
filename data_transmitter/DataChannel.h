#ifndef DATA_CHANNEL_H
#define DATA_CHANNEL_H

#include <string>
#include <memory>
#include "DataChannelProcessesManager.h"

// Forward declarations to avoid circular imports
class DataTransmitter;
class DataTransmitterManager;

class DataChannel {
public:
    DataChannel();
    DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak);
    DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak, const std::string& address);

    bool publish();

    void setName(const std::string& name);
    void setEventsBeforeBreak(int eventsBeforeBreak);
    void setEventsToIgnoreInBreak(int eventsToIgnoreInBreak);
    void setAddress(const std::string& address);

    const std::string& getName() const;
    int getEventsBeforeBreak() const;
    int getEventsToIgnoreInBreak() const;
    const std::string& getAddress() const;
    int getEventsPublished() const;
    int getEventsSeen() const;
    bool isOnBreak() const;
    int getEventsSeenOnBreak() const;
    int getTickTime() const;

    void published();
    void seen();
    void reset();
    void printAttributes() const;

    void setDataChannelProcessesManager(DataChannelProcessesManager manager);
    void addProcessToManager(GeneralProcessor* processor);
    void updateTickTime();

private:
    std::string name;
    int eventsBeforeBreak;
    int eventsToIgnoreInBreak;
    std::string address;
    int eventsPublished;
    int eventsSeen;
    bool onBreak;
    int eventsSeenOnBreak;
    std::shared_ptr<DataTransmitter> transmitter;
    DataChannelProcessesManager processesManager;
    int tickTime;

    bool shouldTakeBreak();
    void startBreak();
    void initializeTransmitter();
};

#endif
