#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

class Config {
public:
    static Config& getInstance();

    void loadFromFile(const std::string& filename);
    void loadFromJson(const nlohmann::json& json);
    
    // Getters with type conversion and default values
    std::string getString(const std::string& key, 
                         const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    // API credentials
    std::string getApiKey() const;
    std::string getApiSecret() const;
    
    // Connection settings
    std::string getWsUrl() const;
    std::string getRestUrl() const;
    
    // Trading parameters
    double getMaxOrderSize() const;
    double getMinOrderSize() const;
    int getMaxOpenOrders() const;
    
    // Performance settings
    int getWebSocketThreads() const;
    int getProcessingThreads() const;
    
    // Logging settings
    std::string getLogFile() const;
    std::string getLogLevel() const;

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const;

    nlohmann::json m_config;
};