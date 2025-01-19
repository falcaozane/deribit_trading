#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "order/orderbook.hpp"
#include "api/websocket.hpp"
#include "../types.hpp"

namespace deribit {

class MarketDataManager {
public:
    MarketDataManager(const std::string& wsUrl);
    ~MarketDataManager();

    // Connection management
    void connect();
    void disconnect();
    bool isConnected() const;

    // Subscription management
    void subscribe(const std::string& instrument,
                  bool orderbook = true,
                  bool trades = true,
                  bool ticker = false);
    
    void unsubscribe(const std::string& instrument,
                     bool orderbook = true,
                     bool trades = true,
                     bool ticker = false);

    // Legacy subscription methods (kept for backward compatibility)
    void subscribeToOrderBook(const std::string& instrument);
    void unsubscribeFromOrderBook(const std::string& instrument);

    // Market data access
    std::shared_ptr<OrderBook> getOrderBook(const std::string& instrument);
    
    // Callback registration
    void setOrderBookCallback(OrderBookCallback callback);
    void setMarketDataCallback(MarketDataCallback callback);

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
    OrderBookCallback m_orderBookCallback;
    MarketDataCallback m_marketDataCallback;

    // Thread safety
    mutable std::mutex m_mutex;
    
    // State tracking
    bool m_isConnected;
};

} // namespace deribit