#ifndef DATATRANSMITTER_H
#define DATATRANSMITTER_H

#include <string>
#include <zmq.hpp>
#include <iostream>

class DataTransmitter {
public:
    DataTransmitter(const std::string& zmqAddress);
    ~DataTransmitter();

    bool bind();
    bool publish(const std::string& data);

private:
    zmq::context_t context;
    zmq::socket_t publisher;
    std::string zmqAddress;
};

#endif // DATATRANSMITTER_H
