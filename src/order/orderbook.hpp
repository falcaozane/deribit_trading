#pragma once
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cmath>
#include "order/order.hpp"

struct PriceLevel {
    double totalVolume;
    std::unordered_map<std::string, std::shared_ptr<Order>> orders;

    PriceLevel() : totalVolume(0.0) {}
};

// Define map types to ensure consistency
using BidMap = std::map<double, PriceLevel, std::greater<double>>;
using AskMap = std::map<double, PriceLevel, std::less<double>>;

class OrderBook {
public:
    OrderBook(const std::string& instrument);

    // Order management
    void addOrder(std::shared_ptr<Order> order);
    void removeOrder(const std::string& orderId);
    void modifyOrder(const std::string& orderId, double newPrice, double newAmount);
    std::shared_ptr<Order> getOrder(const std::string& orderId) const;

    // Market data access
    double getBestBid() const;
    double getBestAsk() const;
    double getMidPrice() const;
    double getSpread() const;

    // Market depth
    BidMap getBidLevels() const;
    AskMap getAskLevels() const;
    size_t getDepth(OrderSide side) const;

    // Instrument info
    std::string getInstrument() const { return m_instrument; }

    // Market data updates
    void clear();
    void updateFromSnapshot(const std::map<double, double>& bids,
                          const std::map<double, double>& asks);
    void processIncrementalUpdate(OrderSide side, double price,
                                double newVolume);

private:
    std::string m_instrument;
    BidMap m_bids;      // Sorted high to low
    AskMap m_asks;      // Sorted low to high
    std::unordered_map<std::string, std::shared_ptr<Order>> m_allOrders;
    mutable std::mutex m_mutex;

    // Template helper functions implemented in header
    template<typename MapType>
    void removeOrderFromPriceLevel(std::shared_ptr<Order> order, MapType& levels) {
        auto levelIt = levels.find(order->getPrice());
        if (levelIt != levels.end()) {
            auto& level = levelIt->second;
            level.orders.erase(order->getOrderId());
            level.totalVolume -= order->getRemainingAmount();
            
            if (level.orders.empty() && std::abs(level.totalVolume) < 1e-10) {
                levels.erase(levelIt);
            }
        }
    }

    template<typename MapType>
    void addOrderToPriceLevel(std::shared_ptr<Order> order, MapType& levels) {
        auto& level = levels[order->getPrice()];
        level.orders[order->getOrderId()] = order;
        level.totalVolume += order->getRemainingAmount();
        
        if (std::abs(level.totalVolume) < 1e-10) {
            levels.erase(order->getPrice());
        }
    }
};