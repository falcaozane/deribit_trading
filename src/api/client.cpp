#include "api/client.hpp"
#include <stdexcept>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/sha.h>

DeribitClient::DeribitClient(const std::string& api_key, const std::string& api_secret)
    : m_apiKey(api_key)
    , m_apiSecret(api_secret)
    , m_baseUrl("https://test.deribit.com/api/v2")
    , m_isAuthenticated(false) {
    
    m_curl = curl_easy_init();
    if (!m_curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

DeribitClient::~DeribitClient() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

bool DeribitClient::authenticate() {
    try {
        nlohmann::json params = {
            {"grant_type", "client_credentials"},
            {"client_id", m_apiKey},
            {"client_secret", m_apiSecret}
        };

        auto response = sendRequest("POST", "/public/auth", params);
        m_isAuthenticated = response.contains("access_token");
        return m_isAuthenticated;
    } catch (const std::exception& e) {
        return false;
    }
}

nlohmann::json DeribitClient::placeOrder(const std::string& instrument, const std::string& side,
                                       double price, double amount, const std::string& orderType) {
    if (!m_isAuthenticated) throw std::runtime_error("Not authenticated");

    nlohmann::json params = {
        {"instrument_name", instrument},
        {"side", side},
        {"price", price},
        {"amount", amount},
        {"type", orderType}
    };

    return sendRequest("POST", "/private/buy", params);
}

nlohmann::json DeribitClient::cancelOrder(const std::string& orderId) {
    if (!m_isAuthenticated) throw std::runtime_error("Not authenticated");

    nlohmann::json params = {
        {"order_id", orderId}
    };

    return sendRequest("POST", "/private/cancel", params);
}

nlohmann::json DeribitClient::modifyOrder(const std::string& orderId, double newPrice, double newAmount) {
    if (!m_isAuthenticated) throw std::runtime_error("Not authenticated");

    nlohmann::json params = {
        {"order_id", orderId},
        {"price", newPrice},
        {"amount", newAmount}
    };

    return sendRequest("POST", "/private/edit", params);
}

nlohmann::json DeribitClient::getOrderbook(const std::string& instrument) {
    nlohmann::json params = {
        {"instrument_name", instrument}
    };

    return sendRequest("GET", "/public/get_order_book", params);
}

nlohmann::json DeribitClient::getPositions(const std::string& currency) {
    if (!m_isAuthenticated) throw std::runtime_error("Not authenticated");

    nlohmann::json params = {
        {"currency", currency}
    };

    return sendRequest("GET", "/private/get_positions", params);
}

size_t DeribitClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

nlohmann::json DeribitClient::sendRequest(const std::string& method, const std::string& endpoint,
                                        const nlohmann::json& params) {
    std::string url = m_baseUrl + endpoint;
    std::string response_string;
    
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response_string);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    if (method == "POST") {
        std::string post_data = params.dump();
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_data.c_str());
    }
    
    CURLcode res = curl_easy_perform(m_curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(res));
    }
    
    return nlohmann::json::parse(response_string);
}