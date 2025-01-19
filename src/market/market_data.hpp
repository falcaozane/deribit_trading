#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "order/orderbook.hpp"
#include "api/websocket.hpp"

// Callback type definitions
using MarketDataCallback = std::function<void(const std::string& instrument, 
                                            const std::string& channel,
                                            const nlohmann::json& data)>;

class MarketDataManager {
public:
    MarketDataManager(const std::string& wsUrl);
    ~MarketDataManager();

    // Connection management
    void connect();
    void disconnect();
    bool isConnected() const;

    // Subscription management
    void subscribeToOrderBook(const std::string& instrument);
    void unsubscribeFromOrderBook(const std::string& instrument);
    void subscribeToTrades(const std::string& instrument);
    void unsubscribeFromTrades(const std::string& instrument);
    void subscribeToTicker(const std::string& instrument);
    void unsubscribeFromTicker(const std::string& instrument);

    // Market data access
    std::shared_ptr<OrderBook> getOrderBook(const std::string& instrument);
    
    // Callback registration
    void setOrderBookCallback(MarketDataCallback callback);
    void setTradeCallback(MarketDataCallback callback);
    void setTickerCallback(MarketDataCallback callback);

private:
    // WebSocket message handler
    void handleWebSocketMessage(const std::string& message);
    void processOrderBookUpdate(const std::string& instrument, const nlohmann::json& data);
    void processTradeUpdate(const std::string& instrument, const nlohmann::json& data);
    void processTickerUpdate(const std::string& instrument, const nlohmann::json& data);

    // Internal helper methods
    void initializeOrderBook(const std::string& instrument);
    std::string createSubscriptionChannel(const std::string& instrument, const std::string& type);

    // WebSocket client
    std::unique_ptr<DeribitWebSocket> m_webSocket;
    std::string m_wsUrl;

    // Market data storage
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> m_orderBooks;
    
    // Subscription tracking
    std::unordered_map<std::string, bool> m_subscriptions;

    // Callbacks
    MarketDataCallback m_orderBookCallback;
    MarketDataCallback m_tradeCallback;
    MarketDataCallback m_tickerCallback;

    // Thread safety
    mutable std::mutex m_mutex;
    
    // State tracking
    bool m_isConnected;
};