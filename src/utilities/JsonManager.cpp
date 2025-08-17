#include "JsonManager.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <iostream>
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
    // Use std::filesystem to resolve the real path
    std::filesystem::path sourcePath(__FILE__);

    // Assume config lives in <project_root>/config/config.json
    std::filesystem::path configPath = sourcePath.parent_path().parent_path().parent_path() / "config" / "config.json";

    // Resolve ".." and get absolute path
    std::filesystem::path canonicalPath = std::filesystem::canonical(configPath);

    return canonicalPath.string();
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
