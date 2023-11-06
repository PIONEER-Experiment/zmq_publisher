#ifndef DATATRANSMITTERMANAGER_H
#define DATATRANSMITTERMANAGER_H

#include <string>
#include <zmq.hpp>
#include <iostream>
#include "ProjectPrinter.h"
#include "DataTransmitter.h"
#include <memory> // Include for std::shared_ptr

class DataTransmitterManager {
public:
    DataTransmitterManager();
    DataTransmitterManager(int verbose = 0);
    ~DataTransmitterManager();

    // Add a new zmq-address and create a DataTransmitter for it
    void addZmqAddress(const std::string& zmqAddress);

    // Add a new zmq-address or edit an existing one
    void setZmqAddress(const std::string& zmqAddress, std::shared_ptr<DataTransmitter> transmitter);

    // Get a DataTransmitter for a zmq-address
    std::shared_ptr<DataTransmitter> getTransmitter(const std::string& zmqAddress);

    // Setter for verbosity control
    void setVerbose(int enableVerbose);

    // Static method to get the singleton instance
    static DataTransmitterManager& Instance(int verbose = 0);

private:
    int verbose; // Added verbosity control
    std::map<std::string, std::shared_ptr<DataTransmitter>> transmitterMap;
};

#endif // DATATRANSMITTERMANAGER_H
