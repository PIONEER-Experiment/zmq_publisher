#include "ODBProcessor.h"
#include "ProjectPrinter.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

ODBProcessor::ODBProcessor(int verbose, const CommandRunner& runner)
    : CommandProcessor(verbose, runner) {
    // Additional initialization specific to ODBProcessor, if needed
}

std::vector<std::string> ODBProcessor::getProcessedOutput() {
    std::vector<std::string> result;

    // Execute the odbedit command
    std::string odbOutput = commandRunner.execute();

    std::string extractedJson = extractJsonFromOutput(odbOutput);

    if (!extractedJson.empty()) {
        result.push_back(extractedJson);
    } else {
        ProjectPrinter printer;
        printer.PrintWarning("Failed to grab ODB data.", __LINE__, __FILE__);
    }

    return result;
}

std::string ODBProcessor::extractJsonFromOutput(const std::string& commandOutput) {
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
