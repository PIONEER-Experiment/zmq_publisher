#include "ODBGrabber.h"
#include "ProjectPrinter.h"
#include "CommandManager.h"
#include <iostream>
#include <sstream>
#include <cstdlib> // For getenv

ODBGrabber::ODBGrabber(const char* clientName)
    : client_name_{}, grabInterval(std::chrono::milliseconds(10)) {
    strncpy(client_name_, clientName, NAME_LENGTH);
    odbEditCommandPath = getODBedItCommandPath();
}

ODBGrabber::ODBGrabber(const char* clientName, int intervalMilliseconds)
    : client_name_{}, grabInterval(std::chrono::milliseconds(intervalMilliseconds)) {
    strncpy(client_name_, clientName, NAME_LENGTH);
    lastGrabTime = std::chrono::high_resolution_clock::now() - grabInterval;
    odbEditCommandPath = getODBedItCommandPath();
}

bool ODBGrabber::isReadyToGrab() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return grabInterval.count() == 0 || (currentTime - lastGrabTime >= grabInterval);
}

bool ODBGrabber::grabODB() {
    CommandManager odbCommand({odbEditCommandPath, "-c", "json", "-q"});

    // Execute the odbedit command
    std::string odbOutput = odbCommand.execute();

    std::string extractedJson = extractJsonFromOutput(odbOutput);

    if (!extractedJson.empty()) {
        odbJson = extractedJson;
        lastGrabTime = std::chrono::high_resolution_clock::now();
        return true;
    } else {
        ProjectPrinter printer;
        printer.PrintError("Failed to grab ODB data.", __LINE__, __FILE__);
        return false;
    }
}

bool ODBGrabber::connectAndGrabODB() {
    if (!getEnvironment()) {
        ProjectPrinter printer;
        printer.PrintError("Failed to get the environment.", __LINE__, __FILE__);
        return false;
    }

    if (!connectToExperiment()) {
        ProjectPrinter printer;
        printer.PrintError("Failed to connect to the experiment.", __LINE__, __FILE__);
        return false;
    }

    // Get the ODB
    midas::odb exp("/");
    odbJson = exp.dump();

    if (!disconnectFromExperiment()) {
        ProjectPrinter printer; 
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
    return true; 
}

std::string ODBGrabber::extractJsonFromOutput(const std::string& commandOutput) {
    size_t found = commandOutput.find("json: {");
    if (found != std::string::npos) {
        std::istringstream jsonStream(commandOutput.substr(found));
        std::string jsonResult;

        std::string line;
        bool skipLastLine = false;

        while (std::getline(jsonStream, line)) {
            if (!skipLastLine && !line.empty()) {
                jsonResult += line + '\n';
            }

            if (line.empty()) {
                skipLastLine = true;
            }
        }

        return jsonResult;
    }

    return "";
}

std::string ODBGrabber::getODBedItCommandPath() {
    char* midasSysPath = std::getenv("MIDASSYS");
    if (midasSysPath) {
        return std::string(midasSysPath) + "/bin/odbedit";
    } else {
        ProjectPrinter printer;
        printer.PrintError("MIDASSYS environment variable not defined.", __LINE__, __FILE__);
        // Handle the case where MIDASSYS is not defined
        return "";
    }
}

