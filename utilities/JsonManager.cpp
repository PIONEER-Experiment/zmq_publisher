#include "JsonManager.h"
#include <fstream>
#include <stdexcept>
#include <ProjectPrinter.h>

nlohmann::json JsonManager::config;

JsonManager& JsonManager::getInstance(const std::string& configFile) {
    static JsonManager instance(configFile);
    return instance;
}

JsonManager& JsonManager::getInstance() {
    static JsonManager instance;
    return instance;
}

JsonManager::JsonManager() : JsonManager(getConfigFilePath()) {
    // The first constructor calls the second constructor with the default config file path
}

JsonManager::JsonManager(const std::string& configFile) : configFilePath(configFile) {
    loadConfigFile();
}

void JsonManager::loadConfigFile() {
    // Load the JSON configuration from the file and replace environment variables
    config = readConfigFile(configFilePath);
    config = replaceEnvironmentVariables(config);
}

const nlohmann::json& JsonManager::getConfig() const {
    return config;
}

void JsonManager::recursivelyReplacePlaceholders(nlohmann::json& json) {
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (it->is_string()) {
            *it = replacePlaceholder(it->get<std::string>());
        } else if (it->is_object()) {
            recursivelyReplacePlaceholders(*it);
        } else if (it->is_array()) {
            for (auto& element : *it) {
                recursivelyReplacePlaceholders(element);
            }
        }
    }
}

nlohmann::json JsonManager::replaceEnvironmentVariables(const nlohmann::json& jsonConfig) {
    ProjectPrinter printer;
    nlohmann::json replacedConfig = jsonConfig;

    recursivelyReplacePlaceholders(replacedConfig);

    return replacedConfig;
}




std::string JsonManager::replacePlaceholder(const std::string& input) {
    std::string result = input;
    size_t startPos = 0;
    size_t openBracket = result.find("$(", startPos);

    while (openBracket != std::string::npos) {
        size_t closeBracket = result.find(")", openBracket);

        if (closeBracket != std::string::npos) {
            std::string envVarName = result.substr(openBracket + 2, closeBracket - openBracket - 2);
            char* envVarValue = std::getenv(envVarName.c_str());

            if (envVarValue != nullptr) {
                result.replace(openBracket, closeBracket - openBracket + 1, envVarValue);
            } else {
                // Handle the case when the environment variable is not found
                throw std::runtime_error("Environment variable not found: " + envVarName);
            }

            startPos = openBracket + 1;
        } else {
            // Handle the case when there's a placeholder without a closing bracket
            throw std::runtime_error("Invalid placeholder in the configuration: " + input);
        }

        openBracket = result.find("$(", startPos);
    }

    return result;
}

std::string JsonManager::getConfigFilePath() {
    // Get the directory of the source file
    std::string sourceDirectory = __FILE__;
    size_t lastSlash = sourceDirectory.find_last_of('/');
    if (lastSlash != std::string::npos) {
        sourceDirectory = sourceDirectory.substr(0, lastSlash);
        lastSlash = sourceDirectory.find_last_of('/');
        if (lastSlash != std::string::npos) {
            sourceDirectory = sourceDirectory.substr(0, lastSlash);
        }
    }

    // Build the full path to the configuration file
    return sourceDirectory + "/config.json";
}

nlohmann::json JsonManager::readConfigFile(const std::string& configFile) {
    nlohmann::json config;
    std::ifstream configFileStream(configFile);
    
    if (!configFileStream) {
        std::string errorMessage = "Failed to open configuration file: " + configFile;
        // Handle the error as needed
        throw std::runtime_error(errorMessage);
    } else {
        try {
            configFileStream >> config;
            configFileStream.close();
        } catch (const nlohmann::json::exception& e) {
            std::string errorMessage = "Error while parsing JSON: " + std::string(e.what());
            // Handle the error as needed
            throw std::runtime_error(errorMessage);
        }
    }
    
    return config;
}
