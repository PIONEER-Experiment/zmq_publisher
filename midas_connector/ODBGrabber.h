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
    bool connectAndGrabODB();
    const std::string& getODBJson() const;

private:
    char client_name_[NAME_LENGTH];
    char host_name_[NAME_LENGTH];
    char experiment_name_[NAME_LENGTH];
    std::string odbJson;
    std::chrono::milliseconds grabInterval;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastGrabTime;
    std::string odbEditCommandPath;

    bool getEnvironment();
    bool connectToExperiment();
    bool disconnectFromExperiment();
    std::string extractJsonFromOutput(const std::string& commandOutput);
    std::string getODBedItCommandPath();

};

#endif // ODBGRABBER_H
