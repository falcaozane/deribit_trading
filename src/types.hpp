#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace deribit {

// Basic enums
enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    LIMIT,
    MARKET,
    STOP_LIMIT,
    STOP_MARKET
};

enum class OrderStatus {
    PENDING,
    OPEN,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

enum class InstrumentType {
    SPOT,
    FUTURE,
    OPTION
};

// Market data structures
struct OrderBookLevel {
    double price;
    double amount;
    int orderCount;

    OrderBookLevel(double p = 0.0, double a = 0.0, int c = 0)
        : price(p), amount(a), orderCount(c) {}
};

struct PriceLevel {
    double price;
    double amount;
    int count;

    PriceLevel(double p = 0.0, double a = 0.0, int c = 0)
        : price(p), amount(a), count(c) {}
};

// Order and trade structures
struct OrderRequest {
    std::string instrument;
    Side side;
    OrderType type;
    double price;
    double amount;
    double stopPrice;     // Optional, for stop orders
    bool reduceOnly;      // Optional
    bool postOnly;        // Optional
    std::string label;    // Optional
    std::chrono::system_clock::time_point timeInForce; // Optional
};

struct OrderResponse {
    std::string orderId;
    OrderStatus status;
    std::string error;
    nlohmann::json raw;
};

struct Trade {
    std::string instrument;
    std::string tradeId;
    Side side;
    double price;
    double amount;
    std::string orderId;  // Optional
    std::string matchingId;  // Optional
    std::chrono::system_clock::time_point timestamp;
    nlohmann::json raw;
};

struct Position {
    std::string instrument;
    double size;
    double entryPrice;
    double liquidationPrice;
    double unrealizedPnl;
    double realizedPnl;
    std::chrono::system_clock::time_point timestamp;
    nlohmann::json raw;
};

// WebSocket response types
struct WSResponse {
    bool success;
    std::string channel;
    nlohmann::json data;
    std::string error;
    int64_t requestId;
};

// Callback types
using OrderBookCallback = std::function<void(const std::string& instrument, 
                                           const std::string& channel,
                                           const nlohmann::json& data)>;

using MarketDataCallback = std::function<void(const std::string& instrument,
                                            const std::string& channel,
                                            const nlohmann::json& data)>;

using OrderCallback = std::function<void(const OrderResponse& response)>;
using TradeCallback = std::function<void(const Trade& trade)>;
using PositionCallback = std::function<void(const Position& position)>;
using ErrorCallback = std::function<void(const std::string& error)>;

// Helper functions
namespace utils {
    inline std::string sideToString(Side side) {
        switch (side) {
            case Side::BUY: return "buy";
            case Side::SELL: return "sell";
            default: return "unknown";
        }
    }

    inline Side stringToSide(const std::string& str) {
        if (str == "buy") return Side::BUY;
        if (str == "sell") return Side::SELL;
        throw std::invalid_argument("Invalid side: " + str);
    }

    inline std::string orderTypeToString(OrderType type) {
        switch (type) {
            case OrderType::LIMIT: return "limit";
            case OrderType::MARKET: return "market";
            case OrderType::STOP_LIMIT: return "stop_limit";
            case OrderType::STOP_MARKET: return "stop_market";
            default: return "unknown";
        }
    }

    inline OrderType stringToOrderType(const std::string& str) {
        if (str == "limit") return OrderType::LIMIT;
        if (str == "market") return OrderType::MARKET;
        if (str == "stop_limit") return OrderType::STOP_LIMIT;
        if (str == "stop_market") return OrderType::STOP_MARKET;
        throw std::invalid_argument("Invalid order type: " + str);
    }

    inline std::string orderStatusToString(OrderStatus status) {
        switch (status) {
            case OrderStatus::PENDING: return "pending";
            case OrderStatus::OPEN: return "open";
            case OrderStatus::PARTIALLY_FILLED: return "partially_filled";
            case OrderStatus::FILLED: return "filled";
            case OrderStatus::CANCELLED: return "cancelled";
            case OrderStatus::REJECTED: return "rejected";
            default: return "unknown";
        }
    }

    inline OrderStatus stringToOrderStatus(const std::string& str) {
        if (str == "pending") return OrderStatus::PENDING;
        if (str == "open") return OrderStatus::OPEN;
        if (str == "partially_filled") return OrderStatus::PARTIALLY_FILLED;
        if (str == "filled") return OrderStatus::FILLED;
        if (str == "cancelled") return OrderStatus::CANCELLED;
        if (str == "rejected") return OrderStatus::REJECTED;
        throw std::invalid_argument("Invalid order status: " + str);
    }
}

} // namespace deribit