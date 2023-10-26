#ifndef ODBGRABBER_H
#define ODBGRABBER_H

#include "midas.h"
#include "msystem.h"
#include "mrpc.h"
#include "mdsupport.h"
#include "midasio.h"
#include "odbxx.h"

#include <cstring>
#include <string>
#include <chrono>

class ODBGrabber {
public:
    ODBGrabber(const char* clientName);
    ODBGrabber(const char* clientName, int intervalMilliseconds);

    bool isReadyToGrab();
    bool grabODB();
    const std::string& getODBJson() const;

private:
    char client_name_[NAME_LENGTH];
    char host_name_[NAME_LENGTH];
    char experiment_name_[NAME_LENGTH];
    std::string odbJson;
    std::chrono::milliseconds grabInterval;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastGrabTime;

    bool getEnvironment();
    bool connectToExperiment();
    bool disconnectFromExperiment();
};

#endif // ODBGRABBER_H
