// DataTransmitter.h
#ifndef DATATRANSMITTER_H
#define DATATRANSMITTER_H

#include <string>
#include <zmq.hpp>
#include <iostream>
#include "ProjectPrinter.h"
#include "DataChannel.h"

/**
 * @brief Transmits data over a ZeroMQ (zmq) publisher socket.
 *
 * The `DataTransmitter` class provides functionality for binding to a zmq publisher socket
 * and publishing data to a specific zmq-address.
 */
class DataTransmitter {
public:
    /**
     * @brief Constructor for DataTransmitter.
     * @param zmqAddress The zmq-address to which the transmitter will bind.
     * @param verbose Verbosity level for logging (default is 0).
     */
    DataTransmitter(const std::string& zmqAddress, int verbose = 0);

    /**
     * @brief Destructor for DataTransmitter.
     */
    ~DataTransmitter();

    /**
     * @brief Binds the transmitter to the zmq publisher socket.
     * @return True if successful, false otherwise.
     */
    bool bind();

    /**
     * @brief Checks if the transmitter is bound to the zmq publisher socket.
     * @return True if bound, false otherwise.
     */
    bool isBound();

    /**
     * @brief Publishes data to the specified data channel.
     * @param dataChannel The data channel to publish to.
     * @param data The data to publish.
     * @return True if successful (this does not necessarily mean data is published), 
     * false otherwise.
     */
    bool publish(DataChannel& dataChannel, const std::string& data);

    /**
     * @brief Sets the verbosity level for logging.
     * @param enableVerbose Verbosity level to set.
     */
    void setVerbose(int enableVerbose);

private:
    zmq::context_t context; ///< ZeroMQ context.
    zmq::socket_t publisher; ///< ZeroMQ publisher socket.
    std::string zmqAddress; ///< The zmq-address to which the transmitter is bound.
    int verbose; ///< Verbosity level for logging.
    bool isBoundToSocket; ///< Flag indicating if the transmitter is bound to the zmq publisher socket.
};

#endif // DATATRANSMITTER_H
