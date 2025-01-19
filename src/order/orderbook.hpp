#pragma once
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "order/order.hpp"

struct PriceLevel {
    double totalVolume;
    std::unordered_map<std::string, std::shared_ptr<Order>> orders;

    PriceLevel() : totalVolume(0.0) {}
};

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
    std::map<double, PriceLevel> getBidLevels() const;
    std::map<double, PriceLevel> getAskLevels() const;
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
    std::map<double, PriceLevel, std::greater<double>> m_bids;  // Sorted high to low
    std::map<double, PriceLevel, std::less<double>> m_asks;     // Sorted low to high
    std::unordered_map<std::string, std::shared_ptr<Order>> m_allOrders;
    mutable std::mutex m_mutex;

    void removeOrderFromPriceLevel(std::shared_ptr<Order> order,
                                 std::map<double, PriceLevel>& levels);
    void addOrderToPriceLevel(std::shared_ptr<Order> order,
                            std::map<double, PriceLevel>& levels);
};