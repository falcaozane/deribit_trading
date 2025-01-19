#include "market/market_data.hpp"
#include <iostream>
#include <sstream>

MarketDataManager::MarketDataManager(const std::string& wsUrl)
    : m_wsUrl(wsUrl)
    , m_isConnected(false) {
    
    m_webSocket = std::make_unique<DeribitWebSocket>();
    m_webSocket->setMessageCallback([this](const std::string& msg) {
        this->handleWebSocketMessage(msg);
    });
}

MarketDataManager::~MarketDataManager() {
    disconnect();
}

void MarketDataManager::connect() {
    try {
        m_webSocket->connect(m_wsUrl);
        m_isConnected = true;
    } catch (const std::exception& e) {
        std::cerr << "WebSocket connection failed: " << e.what() << std::endl;
        m_isConnected = false;
        throw;
    }
}

void MarketDataManager::disconnect() {
    if (m_isConnected) {
        m_webSocket->close();
        m_isConnected = false;
    }
}

bool MarketDataManager::isConnected() const {
    return m_isConnected;
}

void MarketDataManager::subscribeToOrderBook(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Initialize order book if it doesn't exist
    initializeOrderBook(instrument);
    
    // Create subscription channel
    std::string channel = createSubscriptionChannel(instrument, "book");
    
    // Subscribe if not already subscribed
    if (!m_subscriptions[channel]) {
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }
}

void MarketDataManager::unsubscribeFromOrderBook(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string channel = createSubscriptionChannel(instrument, "book");
    if (m_subscriptions[channel]) {
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
}

void MarketDataManager::subscribeToTrades(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string channel = createSubscriptionChannel(instrument, "trades");
    
    if (!m_subscriptions[channel]) {
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }
}

void MarketDataManager::unsubscribeFromTrades(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string channel = createSubscriptionChannel(instrument, "trades");
    
    if (m_subscriptions[channel]) {
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
}

void MarketDataManager::subscribeToTicker(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string channel = createSubscriptionChannel(instrument, "ticker");
    
    if (!m_subscriptions[channel]) {
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }
}

void MarketDataManager::unsubscribeFromTicker(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string channel = createSubscriptionChannel(instrument, "ticker");
    
    if (m_subscriptions[channel]) {
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
}

std::shared_ptr<OrderBook> MarketDataManager::getOrderBook(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_orderBooks.find(instrument);
    return (it != m_orderBooks.end()) ? it->second : nullptr;
}

void MarketDataManager::setOrderBookCallback(MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_orderBookCallback = callback;
}

void MarketDataManager::setTradeCallback(MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tradeCallback = callback;
}

void MarketDataManager::setTickerCallback(MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tickerCallback = callback;
}

void MarketDataManager::handleWebSocketMessage(const std::string& message) {
    try {
        nlohmann::json json = nlohmann::json::parse(message);
        
        // Check if it's a subscription message
        if (json.contains("method") && json["method"] == "subscription") {
            auto params = json["params"];
            auto channel = params["channel"].get<std::string>();
            auto data = params["data"];
            
            // Parse channel to get instrument and type
            std::string instrument, type;
            size_t pos = channel.find('.');
            if (pos != std::string::npos) {
                instrument = channel.substr(0, pos);
                type = channel.substr(pos + 1);
            }
            
            // Process based on message type
            if (type.find("book") != std::string::npos) {
                processOrderBookUpdate(instrument, data);
                if (m_orderBookCallback) {
                    m_orderBookCallback(instrument, "book", data);
                }
            } else if (type.find("trades") != std::string::npos) {
                processTradeUpdate(instrument, data);
                if (m_tradeCallback) {
                    m_tradeCallback(instrument, "trades", data);
                }
            } else if (type.find("ticker") != std::string::npos) {
                processTickerUpdate(instrument, data);
                if (m_tickerCallback) {
                    m_tickerCallback(instrument, "ticker", data);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing WebSocket message: " << e.what() << std::endl;
    }
}

void MarketDataManager::processOrderBookUpdate(const std::string& instrument, 
                                             const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto orderbook = getOrderBook(instrument);
    if (!orderbook) {
        return;
    }
    
    // Check if it's a snapshot or update
    bool isSnapshot = data.contains("type") && data["type"] == "snapshot";
    
    if (isSnapshot) {
        // Process full orderbook snapshot
        std::map<double, double> bids;
        std::map<double, double> asks;
        
        for (const auto& bid : data["bids"]) {
            bids[bid[0].get<double>()] = bid[1].get<double>();
        }
        
        for (const auto& ask : data["asks"]) {
            asks[ask[0].get<double>()] = ask[1].get<double>();
        }
        
        orderbook->updateFromSnapshot(bids, asks);
    } else {
        // Process incremental update
        if (data.contains("changes")) {
            for (const auto& change : data["changes"]) {
                std::string side = change[0].get<std::string>();
                double price = change[1].get<double>();
                double amount = change[2].get<double>();
                
                OrderSide orderSide = (side == "buy") ? OrderSide::BUY : OrderSide::SELL;
                orderbook->processIncrementalUpdate(orderSide, price, amount);
            }
        }
    }
}

void MarketDataManager::processTradeUpdate(const std::string& instrument, 
                                         const nlohmann::json& data) {
    // Implement trade processing logic if needed
}

void MarketDataManager::processTickerUpdate(const std::string& instrument, 
                                          const nlohmann::json& data) {
    // Implement ticker processing logic if needed
}

void MarketDataManager::initializeOrderBook(const std::string& instrument) {
    if (m_orderBooks.find(instrument) == m_orderBooks.end()) {
        m_orderBooks[instrument] = std::make_shared<OrderBook>(instrument);
    }
}

std::string MarketDataManager::createSubscriptionChannel(const std::string& instrument, 
                                                       const std::string& type) {
    std::ostringstream oss;
    oss << instrument << "." << type;
    return oss.str();
}