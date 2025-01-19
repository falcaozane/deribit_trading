#pragma once
#include <string>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class DeribitClient {
public:
    DeribitClient(const std::string& api_key, const std::string& api_secret);
    ~DeribitClient();

    // Authentication
    bool authenticate();

    // Order Management
    nlohmann::json placeOrder(const std::string& instrument, const std::string& side, 
                             double price, double amount, const std::string& orderType);
    nlohmann::json cancelOrder(const std::string& orderId);
    nlohmann::json modifyOrder(const std::string& orderId, double newPrice, double newAmount);
    nlohmann::json getOrderbook(const std::string& instrument);
    nlohmann::json getPositions(const std::string& currency);

private:
    // HTTP request methods
    nlohmann::json sendRequest(const std::string& method, const std::string& endpoint, 
                              const nlohmann::json& params = nlohmann::json());
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string m_apiKey;
    std::string m_apiSecret;
    std::string m_baseUrl;
    CURL* m_curl;
    bool m_isAuthenticated;
};