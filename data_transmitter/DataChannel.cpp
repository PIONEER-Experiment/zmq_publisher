#include "DataChannel.h"
#include "ProjectPrinter.h"
#include "DataTransmitterManager.h"
#include "DataTransmitter.h"

// Constructors
DataChannel::DataChannel()
    : name(""), eventsBeforeBreak(1), eventsToIgnoreInBreak(0), address(""),
      eventsPublished(0), eventsSeen(0), onBreak(false), eventsSeenOnBreak(0) {
}

DataChannel::DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak)
    : name(name), eventsBeforeBreak(eventsBeforeBreak), eventsToIgnoreInBreak(eventsToIgnoreInBreak), address(""),
      eventsPublished(0), eventsSeen(0), onBreak(false), eventsSeenOnBreak(0) {
}

DataChannel::DataChannel(const std::string& name, int eventsBeforeBreak, int eventsToIgnoreInBreak, const std::string& address)
    : name(name), eventsBeforeBreak(eventsBeforeBreak), eventsToIgnoreInBreak(eventsToIgnoreInBreak), address(address),
      eventsPublished(0), eventsSeen(0), onBreak(false), eventsSeenOnBreak(0) {
    initializeTransmitter();
}

bool DataChannel::publish() {
    if (!transmitter->isBound()) {
        if (!transmitter->bind()) {
            return false;
        }
    }

    // Check if the manager exists
    if (processesManager) {
        // Run the processes and add the output to the data buffer
        if (processesManager->runProcesses()) { // Will return false if the eventBuffer was not changed
            // Get the serialized data from the data buffer
            std::string serializedData = processesManager->getDataBuffer().SerializeBuffer();
            return transmitter->publish(*this, serializedData);
        }
    }

    return false;
}

void DataChannel::setName(const std::string& name) {
    this->name = name;
}

void DataChannel::setEventsBeforeBreak(int eventsBeforeBreak) {
    this->eventsBeforeBreak = eventsBeforeBreak;
}

void DataChannel::setEventsToIgnoreInBreak(int eventsToIgnoreInBreak) {
    this->eventsToIgnoreInBreak = eventsToIgnoreInBreak;
}

void DataChannel::setAddress(const std::string& address) {
    this->address = address;
    initializeTransmitter();
}

void DataChannel::setDataChannelProcessesManager(DataChannelProcessesManager* manager) {
    processesManager = manager;
}

void DataChannel::addProcessToManager(std::shared_ptr<GeneralProcessor> processor) {
    if (processesManager) {
        processesManager->addProcessor(processor);
    } else {
        ProjectPrinter printer;
        printer.PrintWarning("DataChannelProcessesManager is not set. Unable to add processor.", __LINE__, __FILE__);
    }
}

const std::string& DataChannel::getName() const {
    return name;
}

int DataChannel::getEventsBeforeBreak() const {
    return eventsBeforeBreak;
}

int DataChannel::getEventsToIgnoreInBreak() const {
    return eventsToIgnoreInBreak;
}

int DataChannel::getEventsPublished() const {
    return eventsPublished;
}

int DataChannel::getEventsSeen() const {
    return eventsSeen;
}

const std::string& DataChannel::getAddress() const {
    return address;
}

// Check if the data channel is on a break
bool DataChannel::isOnBreak() const {
    return onBreak;
}

// Get the number of events seen on the current break
int DataChannel::getEventsSeenOnBreak() const {
    return eventsSeenOnBreak;
}

// Mark an event as published
void DataChannel::published() {
    eventsPublished++;

    // Check if the data channel should start a break
    if (shouldTakeBreak()) {
        startBreak();
    }

    if (onBreak) {
        eventsSeenOnBreak++;
    }
}

// Mark an event as seen
void DataChannel::seen() {
    eventsSeen++;

    // If we are on a break and have seen enough events to ignore, exit the break
    if (onBreak) {
        eventsSeenOnBreak++;
        if (eventsSeenOnBreak >= eventsToIgnoreInBreak) {
            onBreak = false;
            eventsSeenOnBreak = 0;
        }
    }
}

// Check if the data channel should take a break
bool DataChannel::shouldTakeBreak() {
    return (eventsPublished % eventsBeforeBreak == 0) && (eventsPublished > 0);
}

// Start a break
void DataChannel::startBreak() {
    onBreak = true;
    eventsSeenOnBreak = 0;
}

// Reset all attributes
void DataChannel::reset() {
    eventsPublished = 0;
    eventsSeen = 0;
    onBreak = false;
    eventsSeenOnBreak = 0;
}

void DataChannel::printAttributes() const {
    ProjectPrinter printer;
    std::string attributes;

    attributes += "Data Channel Attributes:\n";
    attributes += "Name: " + name + "\n";
    attributes += "Events Before Break: " + std::to_string(eventsBeforeBreak) + "\n";
    attributes += "Events To Ignore In Break: " + std::to_string(eventsToIgnoreInBreak) + "\n";
    attributes += "Events Published: " + std::to_string(eventsPublished) + "\n";
    attributes += "Events Seen: " + std::to_string(eventsSeen) + "\n";

    if (onBreak) {
        attributes += "On Break: true\n";
    } else {
        attributes += "On Break: false\n";
    }

    attributes += "Events Seen On Break: " + std::to_string(eventsSeenOnBreak);

    printer.Print(attributes);
}

void DataChannel::initializeTransmitter() {
    // Get the DataTransmitterManager singleton
    DataTransmitterManager& transmitterManager = DataTransmitterManager::Instance();
    transmitter = transmitterManager.getTransmitter(address);
}

