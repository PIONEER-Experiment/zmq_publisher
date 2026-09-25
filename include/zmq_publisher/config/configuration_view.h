#pragma once

#include <initializer_list>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace zmq_publisher {

class ConfigurationView {
public:
    ConfigurationView(const nlohmann::json& value, std::string location);

    const std::string& location() const noexcept { return location_; }
    const nlohmann::json& value() const noexcept { return *value_; }
    bool contains(std::string_view key) const;
    void validate_keys(std::initializer_list<std::string_view> allowed) const;
    ConfigurationView object(std::string_view key) const;

    template <typename T>
    T required(std::string_view key) const {
        const auto name = std::string(key);
        if (!value_->contains(name)) {
            throw std::invalid_argument(location_ + " requires field '" + name + "'");
        }
        try {
            return value_->at(name).get<T>();
        } catch (const nlohmann::json::exception& error) {
            throw std::invalid_argument(
                location_ + "." + name + " has an invalid value: " + error.what());
        }
    }

    template <typename T>
    T optional(std::string_view key, T fallback) const {
        return contains(key) ? required<T>(key) : std::move(fallback);
    }

private:
    const nlohmann::json* value_;
    std::string location_;
};

}  // namespace zmq_publisher
