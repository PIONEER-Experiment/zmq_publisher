#ifndef PROJECTPRINTER_H
#define PROJECTPRINTER_H

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>

/**
 * @brief A utility class for printing messages with various configurations.
 *
 * The `ProjectPrinter` class provides functionality to print messages with customizable colors,
 * prefixes, suffixes, and additional information such as line numbers and timestamps.
 */
class ProjectPrinter {
public:
    /**
     * @brief Default constructor for ProjectPrinter.
     */
    ProjectPrinter();

    /**
     * @brief Constructor for ProjectPrinter with configuration based on a flag.
     * @param useConfig A flag indicating whether to use default or custom configuration.
     */
    ProjectPrinter(bool useConfig);

    /**
     * @brief Constructor for ProjectPrinter with a custom configuration file.
     * @param configPath The path to the custom configuration file.
     */
    ProjectPrinter(const std::string& configPath);

    /**
     * @brief Prints a message with optional line number and filename information.
     * @param message The message to be printed.
     * @param lineNumber The line number information (does not print if not specified).
     * @param filename The filename information (does not print if not specified).
     */
    void Print(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;

    /**
     * @brief Prints a warning message with optional line number and filename information.
     * @param message The warning message to be printed.
     * @param lineNumber The line number information (does not print if not specified).
     * @param filename The filename information (does not print if not specified).
     */
    void PrintWarning(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;

    /**
     * @brief Prints an error message with optional line number and filename information.
     * @param message The error message to be printed.
     * @param lineNumber The line number information (does not print if not specified).
     * @param filename The filename information (does not print if not specified).
     */
    void PrintError(const std::string& message, int lineNumber = 0, const std::string& filename = "") const;

    /**
     * @brief Prints a message with color and optional line number and filename information.
     * @param message The message to be printed.
     * @param status The status or color of the message.
     * @param lineNumber The line number information (does not print if not specified).
     * @param filename The filename information (does not print if not specified).
     * @param color The color code for the message (default is no color).
     */
    void PrintWithColor(const std::string& message, const std::string& status, int lineNumber, const std::string& filename, const std::string& color) const;

    // Getters for new local variables
    /**
     * @brief Gets the color for information messages.
     * @return The color code for information messages.
     */
    std::string getInfoColor() const;

    /**
     * @brief Gets the color for warning messages.
     * @return The color code for warning messages.
     */
    std::string getWarningColor() const;

    /**
     * @brief Gets the color for error messages.
     * @return The color code for error messages.
     */
    std::string getErrorColor() const;

    /**
     * @brief Checks if line numbers should be printed.
     * @return True if line numbers should be printed, false otherwise.
     */
    bool shouldPrintLineNumber() const;

    /**
     * @brief Gets the prefix for printed messages.
     * @return The prefix for printed messages.
     */
    std::string getPrefix() const;

    /**
     * @brief Gets the suffix for printed messages.
     * @return The suffix for printed messages.
     */
    std::string getSuffix() const;

    /**
     * @brief Gets the configuration as a JSON object.
     * @return The configuration as a JSON object.
     */
    nlohmann::json getConfig() const;

    /**
     * @brief Checks if timestamps should be printed with messages.
     * @return True if timestamps should be printed, false otherwise.
     */
    bool shouldPrintTime() const;

    // Setters for new local variables
    /**
     * @brief Sets the color for information messages.
     * @param color The new color code for information messages.
     */
    void setInfoColor(const std::string& color);

    /**
     * @brief Sets the color for warning messages.
     * @param color The new color code for warning messages.
     */
    void setWarningColor(const std::string& color);

    /**
     * @brief Sets the color for error messages.
     * @param color The new color code for error messages.
     */
    void setErrorColor(const std::string& color);

    /**
     * @brief Sets whether line numbers should be printed.
     * @param printLineNum True to print line numbers, false otherwise.
     */
    void setPrintLineNumber(bool printLineNum);

    /**
     * @brief Sets the prefix for printed messages.
     * @param newPrefix The new prefix for printed messages.
     */
    void setPrefix(const std::string& newPrefix);

    /**
     * @brief Sets the suffix for printed messages.
     * @param newSuffix The new suffix for printed messages.
     */
    void setSuffix(const std::string& newSuffix);

    /**
     * @brief Sets the configuration using a JSON object.
     * @param newConfig The new configuration as a JSON object.
     */
    void setConfig(const nlohmann::json& newConfig);

    /**
     * @brief Sets whether timestamps should be printed with messages.
     * @param printTime True to print timestamps, false otherwise.
     */
    void setPrintTime(bool printTime);

private:
    // ... Other private member functions ...

    /**
     * @brief Initializes the ProjectPrinter with the given configuration file.
     * @param configPath The path to the custom configuration file.
     */
    void Initialize(const std::string& configPath);

    /**
     * @brief Gets the default path for the configuration file.
     * @return The default path for the configuration file.
     */
    std::string getDefaultConfigPath() const;

    /**
     * @brief Builds the complete message string with color, prefix, suffix, and additional information.
     * @param message The original message.
     * @param status The status or color of the message.
     * @param lineNumber The line number information.
     * @param filename The filename information.
     * @return The complete message string.
     */
    std::string buildMessageString(const std::string& message, const std::string& status, int lineNumber, const std::string& filename) const;

    /**
     * @brief Applies color to a string based on the provided color code.
     * @param message The original string.
     * @param color The color code.
     * @return The colored string.
     */
    std::string colorizeString(const std::string& message, const std::string& color) const;

    /**
     * @brief Gets the ANSI color code for the provided color.
     * @param color The color identifier.
     * @return The ANSI color code.
     */
    std::string getColorCode(const std::string& color) const;

    /**
     * @brief Gets the current time as a string.
     * @return The current time as a string.
     */
    std::string getCurrentTime() const;

    nlohmann::json config;          ///< JSON object for storing configuration settings.
    std::string infoColor;          ///< Color code for information messages.
    std::string warningColor;       ///< Color code for warning messages.
    std::string errorColor;         ///< Color code for error messages.
    bool printLineNumber;           ///< Flag indicating whether to print line numbers.
    std::string prefix;             ///< Prefix for printed messages.
    std::string suffix;             ///< Suffix for printed messages.
    bool printTime;                 ///< Flag indicating whether to print timestamps.
};

#endif // PROJECTPRINTER_H
