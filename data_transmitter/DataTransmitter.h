#ifndef DATATRANSMITTER_H
#define DATATRANSMITTER_H

#include <string>
#include <zmq.hpp>
#include <iostream>
#include "ProjectPrinter.h"

class DataTransmitter {
public:
    DataTransmitter(const std::string& zmqAddress, int verbose = 0);
    ~DataTransmitter();

    bool bind();
    bool publish(const std::string& channel, const std::string& data);

    // Setter for verbosity control
    void setVerbose(int enableVerbose);

private:
    zmq::context_t context;
    zmq::socket_t publisher;
    std::string zmqAddress;
    int verbose; // Added verbosity control
};

#endif // DATATRANSMITTER_H