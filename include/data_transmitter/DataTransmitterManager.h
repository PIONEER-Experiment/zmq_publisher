// DataTransmitterManager.h
#ifndef DATATRANSMITTERMANAGER_H
#define DATATRANSMITTERMANAGER_H

#include <string>
#include <zmq.hpp>
#include <iostream>
#include "data_transmitter/DataTransmitter.h"
#include <memory> // Include for std::shared_ptr

/**
 * @brief Manages data transmitters for different zmq-addresses.
 *
 * The `DataTransmitterManager` class is responsible for managing and providing access to
 * data transmitters associated with specific zmq-addresses. It is designed as a singleton.
 */
class DataTransmitterManager {
public:
    /**
     * @brief Default constructor for DataTransmitterManager.
     */
    DataTransmitterManager();

    /**
     * @brief Constructor for DataTransmitterManager with verbosity control.
     * @param verbose Verbosity level for logging (default is 0).
     */
    DataTransmitterManager(int verbose = 0);

    /**
     * @brief Destructor for DataTransmitterManager.
     */
    ~DataTransmitterManager();

    /**
     * @brief Adds a new zmq-address and creates a DataTransmitter for it.
     * @param zmqAddress The zmq-address to add.
     */
    void addZmqAddress(const std::string& zmqAddress);

    /**
     * @brief Adds a new zmq-address or edits an existing one with a provided DataTransmitter.
     * @param zmqAddress The zmq-address to set or add.
     * @param transmitter The DataTransmitter associated with the zmq-address.
     */
    void setZmqAddress(const std::string& zmqAddress, std::shared_ptr<DataTransmitter> transmitter);

    /**
     * @brief Gets a DataTransmitter for a specified zmq-address.
     * @param zmqAddress The zmq-address for which to retrieve the DataTransmitter.
     * @return Shared pointer to the DataTransmitter associated with the zmq-address.
     */
    std::shared_ptr<DataTransmitter> getTransmitter(const std::string& zmqAddress);

    /**
     * @brief Sets the verbosity level for logging.
     * @param enableVerbose Verbosity level to set.
     */
    void setVerbose(int enableVerbose);

    /**
     * @brief Static method to get the singleton instance of DataTransmitterManager.
     * @param verbose Verbosity level for logging (default is 0).
     * @return Reference to the singleton instance of DataTransmitterManager.
     */
    static DataTransmitterManager& Instance(int verbose = 0);

private:
    int verbose; ///< Verbosity level for logging.
    std::map<std::string, std::shared_ptr<DataTransmitter>> transmitterMap; ///< Map of zmq-addresses to DataTransmitters.
};

#endif // DATATRANSMITTERMANAGER_H
