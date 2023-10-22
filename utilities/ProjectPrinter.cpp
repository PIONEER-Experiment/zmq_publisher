#include "ProjectPrinter.h"
#include <fstream>
#include <iostream>
#include <string>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

ProjectPrinter::ProjectPrinter() {
    Initialize(getDefaultConfigPath());
}

ProjectPrinter::ProjectPrinter(const std::string& configPath) {
    Initialize(configPath);
}

ProjectPrinter::ProjectPrinter(bool useConfig) {
    if (useConfig) {
        // Use the default configuration file
        Initialize(getDefaultConfigPath());
    } else {
        // Set default options here
        infoColor = "white";
        warningColor = "yellow";
        errorColor = "red";
        printLineNumber = true;
        prefix = "";
        suffix = "";
    }
}

void ProjectPrinter::Initialize(const std::string& configPath) {
    std::ifstream configFile(configPath);
    configFile >> config;
    configFile.close(); // Close the configuration file

    // Initialize local variables from the configuration
    infoColor = config["info_color"];
    warningColor = config["warning_color"];
    errorColor = config["error_color"];
    printLineNumber = config["print_line_number"];
    prefix = config["prefix"];
    suffix = config["suffix"];
}

std::string ProjectPrinter::getDefaultConfigPath() const {
    // Get the directory of the source file (ProjectPrinter.cpp)
    std::string sourceDirectory(__FILE__);
    size_t lastSeparator = sourceDirectory.find_last_of('/');
    if (lastSeparator != std::string::npos) {
        sourceDirectory = sourceDirectory.substr(0, lastSeparator + 1); // Include the trailing slash
    } else {
        sourceDirectory = "./"; // If no directory found, assume it's in the current directory
    }

    // Construct the relative path to the default configuration file
    return sourceDirectory + "printer_config.json";
}

void ProjectPrinter::Print(const std::string& message, int lineNumber, const std::string& filename) const {
    PrintWithColor(message, "", lineNumber, filename, config["info_color"]);
}

void ProjectPrinter::PrintWarning(const std::string& message, int lineNumber, const std::string& filename) const {
    PrintWithColor(message, "WARNING", lineNumber, filename, config["warning_color"]);
}

void ProjectPrinter::PrintError(const std::string& message, int lineNumber, const std::string& filename) const {
    PrintWithColor(message, "ERROR", lineNumber, filename, config["error_color"]);
}

void ProjectPrinter::PrintWithColor(const std::string& message, const std::string& status, int lineNumber, const std::string& filename, const std::string& color) const {
    std::string messageString = buildMessageString(message, status, lineNumber, filename);
    std::cout << colorizeString(messageString, color) << std::endl;
}

std::string ProjectPrinter::buildMessageString(const std::string& message, const std::string& status, int lineNumber, const std::string& filename) const {
    std::string messageString;
    std::string trimmedFilename = filename;

    if (!getPrefix().empty() || !filename.empty() || lineNumber > 0) {
        messageString = "[";

        if (!getPrefix().empty()) {
            messageString += getPrefix();
        }

        if (!status.empty()) {
            messageString += ", " + status;
        }

        if (!filename.empty() && lineNumber > 0) {
            // Extract the filename from the path
            size_t lastSeparator = filename.find_last_of('/');
            if (lastSeparator != std::string::npos) {
                trimmedFilename = filename.substr(lastSeparator + 1);
            }
            
            messageString += " at (" + trimmedFilename + ":" + std::to_string(lineNumber) + ")";
        }

        messageString += "] ";
    }

    messageString += message;
    return messageString;
}

std::string ProjectPrinter::colorizeString(const std::string& message, const std::string& color) const {
    std::string colorCode = getColorCode(color);
    std::string resetCode = "\033[0m";
    return colorCode + message + resetCode;
}

std::string ProjectPrinter::getColorCode(const std::string& color) const {
    // Map color names to ANSI color codes
    std::unordered_map<std::string, std::string> colorMap = {
        {"black", "\033[30m"},
        {"red", "\033[31m"},
        {"green", "\033[32m"},
        {"yellow", "\033[33m"},
        {"blue", "\033[34m"},
        {"magenta", "\033[35m"},
        {"cyan", "\033[36m"},
        {"white", "\033[37m"}
    };

    if (colorMap.find(color) != colorMap.end()) {
        return colorMap[color];
    } else {
        return ""; // Return an empty string for unknown colors
    }
}

// Getters for configuration details
std::string ProjectPrinter::getInfoColor() const {
    return infoColor;
}

std::string ProjectPrinter::getWarningColor() const {
    return warningColor;
}

std::string ProjectPrinter::getErrorColor() const {
    return errorColor;
}

bool ProjectPrinter::shouldPrintLineNumber() const {
    return printLineNumber;
}

std::string ProjectPrinter::getPrefix() const {
    return prefix;
}

std::string ProjectPrinter::getSuffix() const {
    return suffix;
}

nlohmann::json ProjectPrinter::getConfig() const {
    return config;
}

// Setters for configuration details
void ProjectPrinter::setInfoColor(const std::string& color) {
    infoColor = color;
}

void ProjectPrinter::setWarningColor(const std::string& color) {
    warningColor = color;
}

void ProjectPrinter::setErrorColor(const std::string& color) {
    errorColor = color;
}

void ProjectPrinter::setPrintLineNumber(bool printLineNum) {
    printLineNumber = printLineNum;
}

void ProjectPrinter::setPrefix(const std::string& newPrefix) {
    prefix = newPrefix;
}

void ProjectPrinter::setSuffix(const std::string& newSuffix) {
    suffix = newSuffix;
}

void ProjectPrinter::setConfig(const nlohmann::json& newConfig) {
    config = newConfig;
}

