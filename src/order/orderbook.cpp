#include "order/orderbook.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

OrderBook::OrderBook(const std::string& instrument)
    : m_instrument(instrument) {}

void OrderBook::addOrder(std::shared_ptr<Order> order) {
    if (!order || order->getInstrument() != m_instrument) {
        throw std::invalid_argument("Invalid order");
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_allOrders.find(order->getOrderId()) != m_allOrders.end()) {
        throw std::invalid_argument("Order already exists");
    }

    m_allOrders[order->getOrderId()] = order;

    if (order->getSide() == OrderSide::BUY) {
        addOrderToPriceLevel(order, m_bids);
    } else {
        addOrderToPriceLevel(order, m_asks);
    }
}

void OrderBook::removeOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto orderIt = m_allOrders.find(orderId);
    if (orderIt == m_allOrders.end()) {
        return;
    }

    auto order = orderIt->second;
    if (order->getSide() == OrderSide::BUY) {
        removeOrderFromPriceLevel(order, m_bids);
    } else {
        removeOrderFromPriceLevel(order, m_asks);
    }

    m_allOrders.erase(orderId);
}

void OrderBook::modifyOrder(const std::string& orderId, double newPrice, double newAmount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto orderIt = m_allOrders.find(orderId);
    if (orderIt == m_allOrders.end()) {
        throw std::invalid_argument("Order not found");
    }

    auto order = orderIt->second;
    
    // Remove from current price level
    if (order->getSide() == OrderSide::BUY) {
        removeOrderFromPriceLevel(order, m_bids);
    } else {
        removeOrderFromPriceLevel(order, m_asks);
    }

    // Update order
    order->setPrice(newPrice);
    order->setAmount(newAmount);

    // Add to new price level
    if (order->getSide() == OrderSide::BUY) {
        addOrderToPriceLevel(order, m_bids);
    } else {
        addOrderToPriceLevel(order, m_asks);
    }
}

std::shared_ptr<Order> OrderBook::getOrder(const std::string& orderId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_allOrders.find(orderId);
    return (it != m_allOrders.end()) ? it->second : nullptr;
}

double OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_bids.empty() ? m_bids.begin()->first : 0.0;
}

double OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_asks.empty() ? m_asks.begin()->first : 0.0;
}

double OrderBook::getMidPrice() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bids.empty() || m_asks.empty()) return 0.0;
    return (m_bids.begin()->first + m_asks.begin()->first) / 2.0;
}

double OrderBook::getSpread() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bids.empty() || m_asks.empty()) return 0.0;
    return m_asks.begin()->first - m_bids.begin()->first;
}

std::map<double, PriceLevel> OrderBook::getBidLevels() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bids;
}

std::map<double, PriceLevel> OrderBook::getAskLevels() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_asks;
}

size_t OrderBook::getDepth(OrderSide side) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (side == OrderSide::BUY) ? m_bids.size() : m_asks.size();
}

void OrderBook::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bids.clear();
    m_asks.clear();
    m_allOrders.clear();
}

void OrderBook::updateFromSnapshot(const std::map<double, double>& bids,
                                 const std::map<double, double>& asks) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear existing price levels but keep active orders
    m_bids.clear();
    m_asks.clear();
    
    // Update bid levels
    for (const auto& [price, volume] : bids) {
        m_bids[price] = PriceLevel();
        m_bids[price].totalVolume = volume;
    }
    
    // Update ask levels
    for (const auto& [price, volume] : asks) {
        m_asks[price] = PriceLevel();
        m_asks[price].totalVolume = volume;
    }
    
    // Re-add active orders to appropriate price levels
    for (const auto& [orderId, order] : m_allOrders) {
        if (order->isActive()) {
            if (order->getSide() == OrderSide::BUY) {
                addOrderToPriceLevel(order, m_bids);
            } else {
                addOrderToPriceLevel(order, m_asks);
            }
        }
    }
}

void OrderBook::processIncrementalUpdate(OrderSide side, double price, double newVolume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& levels = (side == OrderSide::BUY) ? m_bids : m_asks;
    
    if (std::abs(newVolume) < 1e-10) {  // Using small epsilon for floating point comparison
        levels.erase(price);
    } else {
        auto& level = levels[price];
        level.totalVolume = newVolume;
        
        // Adjust orders at this price level if necessary
        double orderVolume = 0.0;
        for (const auto& [orderId, order] : level.orders) {
            orderVolume += order->getRemainingAmount();
        }
        
        // If order volume exceeds total volume, proportionally reduce orders
        if (orderVolume > newVolume) {
            double ratio = newVolume / orderVolume;
            for (const auto& [orderId, order] : level.orders) {
                order->setAmount(order->getAmount() * ratio);
            }
        }
    }
}

void OrderBook::removeOrderFromPriceLevel(std::shared_ptr<Order> order,
                                        std::map<double, PriceLevel>& levels) {
    auto levelIt = levels.find(order->getPrice());
    if (levelIt != levels.end()) {
        auto& level = levelIt->second;
        level.orders.erase(order->getOrderId());
        level.totalVolume -= order->getRemainingAmount();
        
        // Remove price level if no orders remain and volume is zero
        if (level.orders.empty() && std::abs(level.totalVolume) < 1e-10) {
            levels.erase(levelIt);
        }
    }
}

void OrderBook::addOrderToPriceLevel(std::shared_ptr<Order> order,
                                   std::map<double, PriceLevel>& levels) {
    auto& level = levels[order->getPrice()];
    level.orders[order->getOrderId()] = order;
    level.totalVolume += order->getRemainingAmount();
    
    // Ensure price level is removed if no volume remains
    if (std::abs(level.totalVolume) < 1e-10) {
        levels.erase(order->getPrice());
    }
}