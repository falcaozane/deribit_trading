#include "market/market_data.hpp"
#include "utils/logger.hpp"
#include <sstream>

namespace deribit {

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
        auto& logger = Logger::getInstance();
        logger.error("WebSocket connection failed: ", e.what());
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

void MarketDataManager::subscribe(const std::string& instrument,
                                bool orderbook,
                                bool trades,
                                bool ticker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (orderbook) {
        std::string channel = createSubscriptionChannel(instrument, "book");
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }
    
    if (trades) {
        std::string channel = createSubscriptionChannel(instrument, "trades");
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }
    
    if (ticker) {
        std::string channel = createSubscriptionChannel(instrument, "ticker");
        m_webSocket->subscribe(channel);
        m_subscriptions[channel] = true;
    }

    if (orderbook) {
        initializeOrderBook(instrument);
    }
}

void MarketDataManager::unsubscribe(const std::string& instrument,
                                  bool orderbook,
                                  bool trades,
                                  bool ticker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (orderbook) {
        std::string channel = createSubscriptionChannel(instrument, "book");
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
    
    if (trades) {
        std::string channel = createSubscriptionChannel(instrument, "trades");
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
    
    if (ticker) {
        std::string channel = createSubscriptionChannel(instrument, "ticker");
        m_webSocket->unsubscribe(channel);
        m_subscriptions[channel] = false;
    }
}

void MarketDataManager::subscribeToOrderBook(const std::string& instrument) {
    subscribe(instrument, true, false, false);
}

void MarketDataManager::unsubscribeFromOrderBook(const std::string& instrument) {
    unsubscribe(instrument, true, false, false);
}

std::shared_ptr<OrderBook> MarketDataManager::getOrderBook(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_orderBooks.find(instrument);
    return (it != m_orderBooks.end()) ? it->second : nullptr;
}

void MarketDataManager::setOrderBookCallback(OrderBookCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_orderBookCallback = callback;
}

void MarketDataManager::setMarketDataCallback(MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_marketDataCallback = callback;
}

void MarketDataManager::handleWebSocketMessage(const std::string& message) {
    try {
        nlohmann::json json = nlohmann::json::parse(message);
        
        if (json.contains("method") && json["method"] == "subscription") {
            auto params = json["params"];
            auto channel = params["channel"].get<std::string>();
            auto data = params["data"];
            
            // Parse channel to get instrument and type
            std::string instrument, type;
            size_t separator = channel.find('.');
            if (separator != std::string::npos) {
                instrument = channel.substr(0, separator);
                type = channel.substr(separator + 1);
            }
            
            // Route the update to appropriate handler
            if (type.find("book") != std::string::npos) {
                processOrderBookUpdate(instrument, data);
                if (m_orderBookCallback) {
                    m_orderBookCallback(instrument, "book", data);
                }
            } else if (type.find("trades") != std::string::npos || 
                       type.find("ticker") != std::string::npos) {
                if (m_marketDataCallback) {
                    m_marketDataCallback(instrument, type, data);
                }
            }
        }
    } catch (const std::exception& e) {
        auto& logger = Logger::getInstance();
        logger.error("Error processing WebSocket message: ", e.what());
    }
}

void MarketDataManager::processOrderBookUpdate(const std::string& instrument, 
                                             const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto orderbook = getOrderBook(instrument);
    if (!orderbook) return;
    
    if (data.contains("type") && data["type"] == "snapshot") {
        std::map<double, double> bids, asks;
        
        for (const auto& bid : data["bids"]) {
            bids[bid[0].get<double>()] = bid[1].get<double>();
        }
        
        for (const auto& ask : data["asks"]) {
            asks[ask[0].get<double>()] = ask[1].get<double>();
        }
        
        orderbook->updateFromSnapshot(bids, asks);
    } else {
        for (const auto& change : data["changes"]) {
            std::string side = change[0].get<std::string>();
            double price = change[1].get<double>();
            double amount = change[2].get<double>();
            
            orderbook->processIncrementalUpdate(
                side == "buy" ? OrderSide::BUY : OrderSide::SELL,
                price,
                amount
            );
        }
    }
}

void MarketDataManager::processTradeUpdate(const std::string& instrument, 
                                         const nlohmann::json& data) {
    if (m_marketDataCallback) {
        m_marketDataCallback(instrument, "trades", data);
    }
}

void MarketDataManager::processTickerUpdate(const std::string& instrument, 
                                          const nlohmann::json& data) {
    if (m_marketDataCallback) {
        m_marketDataCallback(instrument, "ticker", data);
    }
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

} // namespace deribit