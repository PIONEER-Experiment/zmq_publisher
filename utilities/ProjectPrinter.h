#ifndef PROJECTPRINTER_H
#define PROJECTPRINTER_H

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>


class ProjectPrinter {
public:
    ProjectPrinter();
    ProjectPrinter(bool useConfig);
    ProjectPrinter(const std::string& configPath);
    void Print(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;
    void PrintWarning(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;
    void PrintError(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;
    void PrintWithColor(const std::string& message, const std::string& status, int lineNumber, const std::string& filename, const std::string& color) const;


    // Getters for new local variables
    std::string getInfoColor() const;
    std::string getWarningColor() const;
    std::string getErrorColor() const;
    bool shouldPrintLineNumber() const;
    std::string getPrefix() const;
    std::string getSuffix() const;
    nlohmann::json getConfig() const;
    bool shouldPrintTime() const;

    // Setters for new local variables
    void setInfoColor(const std::string& color);
    void setWarningColor(const std::string& color);
    void setErrorColor(const std::string& color);
    void setPrintLineNumber(bool printLineNum);
    void setPrefix(const std::string& newPrefix);
    void setSuffix(const std::string& newSuffix);
    void setConfig(const nlohmann::json& newConfig);
    void setPrintTime(bool printTime);

private:
    void Initialize(const std::string& configPath);
    std::string getDefaultConfigPath() const;
    std::string buildMessageString(const std::string& message, const std::string& status, int lineNumber, const std::string& filename) const;
    std::string colorizeString(const std::string& message, const std::string& color) const;
    std::string getColorCode(const std::string& color) const;
    std::string getCurrentTime() const;

    nlohmann::json config;
    std::string infoColor;
    std::string warningColor;
    std::string errorColor;
    bool printLineNumber;
    std::string prefix;
    std::string suffix;
    bool printTime;
};

#endif // PROJECTPRINTER_H
