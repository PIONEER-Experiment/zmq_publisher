#include "data_transmitter/DataTransmitterManager.h"

DataTransmitterManager::DataTransmitterManager(int verbose) : verbose(verbose) {}

DataTransmitterManager& DataTransmitterManager::Instance(int verbose) {
    static DataTransmitterManager instance(verbose);
    return instance;
}

DataTransmitterManager::~DataTransmitterManager() {
    // No need to manually delete shared_ptr objects; they will be automatically managed
}

void DataTransmitterManager::addZmqAddress(const std::string& zmqAddress) {
    if (transmitterMap.find(zmqAddress) == transmitterMap.end()) {
        transmitterMap[zmqAddress] = std::make_shared<DataTransmitter>(zmqAddress, verbose);
    }
}

void DataTransmitterManager::setZmqAddress(const std::string& zmqAddress, std::shared_ptr<DataTransmitter> transmitter) {
    transmitterMap[zmqAddress] = transmitter;
}

std::shared_ptr<DataTransmitter> DataTransmitterManager::getTransmitter(const std::string& zmqAddress) {
    if (transmitterMap.find(zmqAddress) != transmitterMap.end()) {
        return transmitterMap[zmqAddress];
    }
    // If it's not there, we'll just make a new one and add it to the map
    addZmqAddress(zmqAddress);
    return transmitterMap[zmqAddress];
}

void DataTransmitterManager::setVerbose(int enableVerbose) {
    verbose = enableVerbose;
}
