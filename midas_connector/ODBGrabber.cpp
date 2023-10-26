#include "ODBGrabber.h"
#include "ProjectPrinter.h"

ODBGrabber::ODBGrabber(const char* clientName)
    : client_name_{}, grabInterval(std::chrono::milliseconds(0)) {
    strncpy(client_name_, clientName, NAME_LENGTH);
}

ODBGrabber::ODBGrabber(const char* clientName, int intervalMilliseconds)
    : client_name_{}, grabInterval(std::chrono::milliseconds(intervalMilliseconds)) {
    strncpy(client_name_, clientName, NAME_LENGTH);
    lastGrabTime = std::chrono::high_resolution_clock::now() - grabInterval;
}

bool ODBGrabber::isReadyToGrab() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return grabInterval.count() == 0 || (currentTime - lastGrabTime >= grabInterval);
}

bool ODBGrabber::grabODB() {
    if (!getEnvironment()) {
        ProjectPrinter printer; // Initialize your ProjectPrinter
        printer.PrintError("Failed to get the environment.", __LINE__, __FILE__);
        return false;
    }

    if (!connectToExperiment()) {
        ProjectPrinter printer; // Initialize your ProjectPrinter
        printer.PrintError("Failed to connect to the experiment.", __LINE__, __FILE__);
        return false;
    }

    // Get the ODB
    midas::odb exp("/");
    odbJson = exp.dump();

    if (!disconnectFromExperiment()) {
        ProjectPrinter printer; // Initialize your ProjectPrinter
        printer.PrintError("Failed to disconnect from the experiment.", __LINE__, __FILE__);
        return false;
    }

    lastGrabTime = std::chrono::high_resolution_clock::now();

    return true;
}

const std::string& ODBGrabber::getODBJson() const {
    return odbJson;
}

bool ODBGrabber::getEnvironment() {
    int status = cm_get_environment(host_name_, sizeof(host_name_), experiment_name_, sizeof(experiment_name_));
    return (status == CM_SUCCESS);
}

bool ODBGrabber::connectToExperiment() {
    int status = cm_connect_experiment(host_name_, experiment_name_, client_name_, NULL);
    return (status == CM_SUCCESS);
}

bool ODBGrabber::disconnectFromExperiment() {
    cm_disconnect_experiment();
    return true; // Disconnect function doesn't return an error
}
