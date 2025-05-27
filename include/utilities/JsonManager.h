#ifndef JSONMANAGER_H
#define JSONMANAGER_H

#include <nlohmann/json.hpp>
#include <string>

/**
 * @brief A utility class for managing JSON configurations.
 *
 * The `JsonManager` class provides functionality to load and retrieve JSON configurations.
 */
class JsonManager {
public:
    /**
     * @brief Gets the singleton instance of JsonManager with a specified config file.
     * @param configFile The path to the JSON configuration file.
     * @return Reference to the singleton instance.
     */
    static JsonManager& getInstance(const std::string& configFile);

    /**
     * @brief Gets the singleton instance of JsonManager with a default config file.
     * @return Reference to the singleton instance.
     */
    static JsonManager& getInstance();

    /**
     * @brief Loads the configuration from the specified JSON file.
     */
    void loadConfigFile();

    /**
     * @brief Gets the current configuration as a const reference.
     * @return Const reference to the JSON configuration.
     */
    const nlohmann::json& getConfig() const;

private:
    /**
     * @brief Default constructor for JsonManager.
     */
    JsonManager();

    /**
     * @brief Constructor for JsonManager with a specified config file.
     * @param configFile The path to the JSON configuration file.
     */
    JsonManager(const std::string& configFile);

    /**
     * @brief Static JSON object to store the configuration.
     */
    static nlohmann::json config;

    /**
     * @brief Replaces environment variables in the JSON configuration.
     * @param jsonConfig The JSON configuration.
     * @return Modified JSON configuration with replaced environment variables.
     */
    static nlohmann::json replaceEnvironmentVariables(const nlohmann::json& jsonConfig);

    /**
     * @brief Replaces placeholders in a string with actual values. A helper function for replaceEnvironmentVariables.
     * @param input The input string with placeholders.
     * @return String with replaced placeholders.
     * @see replaceEnvironmentVariables
     */
    static std::string replacePlaceholder(const std::string& input);

    /**
     * @brief Path to the configuration file.
     */
    std::string configFilePath;

    /**
     * @brief Reads the configuration from the specified JSON file. 
     * @param configFile The path to the JSON configuration file.
     * @return JSON object representing the configuration.
     */
    nlohmann::json readConfigFile(const std::string& configFile);

    /**
     * @brief Gets the path to the configuration file.
     * @return The path to the configuration file.
     */
    std::string getConfigFilePath();

    /**
     * @brief Recursively replaces placeholders in a JSON object. A helper function for replaceEnvironmentVariables.
     * @param json The JSON object to be modified.
     * @see replaceEnvironmentVariables
     */
    static void recursivelyReplacePlaceholders(nlohmann::json& json);
};

#endif // JSONMANAGER_H
