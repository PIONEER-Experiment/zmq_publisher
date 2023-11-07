#ifndef JSONMANAGER_H
#define JSONMANAGER_H

#include <nlohmann/json.hpp>
#include <string>

class JsonManager {
public:
    static JsonManager& getInstance(const std::string& configFile);
    static JsonManager& getInstance();
    void loadConfigFile();
    const nlohmann::json& getConfig() const;

private:
    JsonManager();
    JsonManager(const std::string& configFile);
    static nlohmann::json config;
    static nlohmann::json replaceEnvironmentVariables(const nlohmann::json& jsonConfig);
    static std::string replacePlaceholder(const std::string& input);
    std::string configFilePath;
    nlohmann::json readConfigFile(const std::string& configFile);
    std::string getConfigFilePath();
    static void recursivelyReplacePlaceholders(nlohmann::json& json);
};

#endif // JSONMANAGER_H
