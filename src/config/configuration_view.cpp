#include "zmq_publisher/config/configuration_view.h"

#include <algorithm>

namespace zmq_publisher {

ConfigurationView::ConfigurationView(
    const nlohmann::json& value,
    std::string location)
    : value_(&value), location_(std::move(location)) {
    if (!value.is_object()) {
        throw std::invalid_argument(location_ + " must be an object");
    }
}

bool ConfigurationView::contains(std::string_view key) const {
    return value_->contains(std::string(key));
}

void ConfigurationView::validate_keys(
    std::initializer_list<std::string_view> allowed) const {
    for (const auto& [key, _] : value_->items()) {
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            throw std::invalid_argument(location_ + " contains unknown field '" + key + "'");
        }
    }
}

ConfigurationView ConfigurationView::object(std::string_view key) const {
    const auto name = std::string(key);
    if (!value_->contains(name)) {
        throw std::invalid_argument(location_ + " requires object field '" + name + "'");
    }
    return ConfigurationView(value_->at(name), location_ + "." + name);
}

}  // namespace zmq_publisher
