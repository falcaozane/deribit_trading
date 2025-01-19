#include "utils/config.hpp"
#include <fstream>
#include <stdexcept>

namespace deribit {

namespace {
    // Helper function to ensure thread-safe initialization
    nlohmann::json getDefaultConfig() {
        return {
            {"api_key", "rXHCaw-T"},
            {"api_secret", "294sD3YBhxuKIo6GXiwf3mQ4Oc-U7Bnt9emLhgeLfg0"},
            {"ws_url", "wss://test.deribit.com/ws/api/v2"},
            {"rest_url", "https://test.deribit.com/api/v2"},
            {"max_order_size", 10.0},
            {"min_order_size", 0.0001},
            {"max_open_orders", 100},
            {"websocket_threads", 2},
            {"processing_threads", 4},
            {"log_file", "trading_system.log"},
            {"log_level", "INFO"}
        };
    }
}

Config::Config() : m_config(getDefaultConfig()) {}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

void Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }

    nlohmann::json json;
    file >> json;
    loadFromJson(json);
}

void Config::loadFromJson(const nlohmann::json& json) {
    m_config.merge_patch(json);
}

template<typename T>
T Config::getValue(const std::string& key, const T& defaultValue) const {
    try {
        if (m_config.contains(key)) {
            return m_config[key].get<T>();
        }
    } catch (const nlohmann::json::exception& e) {
        // Log error or handle type conversion failure
    }
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    return getValue<std::string>(key, defaultValue);
}

int Config::getInt(const std::string& key, int defaultValue) const {
    return getValue<int>(key, defaultValue);
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    return getValue<double>(key, defaultValue);
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    return getValue<bool>(key, defaultValue);
}

std::string Config::getApiKey() const {
    return getString("api_key");
}

std::string Config::getApiSecret() const {
    return getString("api_secret");
}

std::string Config::getWsUrl() const {
    return getString("ws_url");
}

std::string Config::getRestUrl() const {
    return getString("rest_url");
}

double Config::getMaxOrderSize() const {
    return getDouble("max_order_size");
}

double Config::getMinOrderSize() const {
    return getDouble("min_order_size");
}

int Config::getMaxOpenOrders() const {
    return getInt("max_open_orders");
}

int Config::getWebSocketThreads() const {
    return getInt("websocket_threads");
}

int Config::getProcessingThreads() const {
    return getInt("processing_threads");
}

std::string Config::getLogFile() const {
    return getString("log_file");
}

std::string Config::getLogLevel() const {
    return getString("log_level");
}

// Template instantiations within the namespace
template int Config::getValue<int>(const std::string&, const int&) const;
template double Config::getValue<double>(const std::string&, const double&) const;
template bool Config::getValue<bool>(const std::string&, const bool&) const;
template std::string Config::getValue<std::string>(const std::string&, const std::string&) const;

} // namespace deribit