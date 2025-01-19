#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace deribit {

class Config {
public:
    static Config& getInstance();

    // Configuration loading
    void loadFromFile(const std::string& filename);
    void loadFromJson(const nlohmann::json& json);
    
    // Generic value getters
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const;
    
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    // Specific getters
    std::string getApiKey() const;
    std::string getApiSecret() const;
    std::string getWsUrl() const;
    std::string getRestUrl() const;
    double getMaxOrderSize() const;
    double getMinOrderSize() const;
    int getMaxOpenOrders() const;
    int getWebSocketThreads() const;
    int getProcessingThreads() const;
    std::string getLogFile() const;
    std::string getLogLevel() const;

private:
    Config();  // Private constructor for singleton
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    nlohmann::json m_config;
};

} // namespace deribit