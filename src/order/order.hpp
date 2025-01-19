#pragma once
#include <string>
#include <chrono>
#include <memory>

enum class OrderSide {
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
    FILLED,
    PARTIALLY_FILLED,
    CANCELLED,
    REJECTED
};

class Order {
public:
    Order(const std::string& instrument, OrderSide side, OrderType type, 
          double price, double amount);
    
    // Getters
    std::string getOrderId() const { return m_orderId; }
    std::string getInstrument() const { return m_instrument; }
    OrderSide getSide() const { return m_side; }
    OrderType getType() const { return m_type; }
    double getPrice() const { return m_price; }
    double getAmount() const { return m_amount; }
    double getFilledAmount() const { return m_filledAmount; }
    double getRemainingAmount() const { return m_amount - m_filledAmount; }
    OrderStatus getStatus() const { return m_status; }
    std::chrono::system_clock::time_point getCreationTime() const { return m_creationTime; }
    std::chrono::system_clock::time_point getLastUpdateTime() const { return m_lastUpdateTime; }

    // Setters
    void setOrderId(const std::string& orderId) { m_orderId = orderId; }
    void setStatus(OrderStatus status);
    void setFilledAmount(double amount);
    void setPrice(double price);
    void setAmount(double amount);
    
    // Utility functions
    bool isFilled() const { return m_status == OrderStatus::FILLED; }
    bool isActive() const;
    void updateLastUpdateTime() { m_lastUpdateTime = std::chrono::system_clock::now(); }

private:
    std::string m_orderId;
    std::string m_instrument;
    OrderSide m_side;
    OrderType m_type;
    double m_price;
    double m_amount;
    double m_filledAmount;
    OrderStatus m_status;
    std::chrono::system_clock::time_point m_creationTime;
    std::chrono::system_clock::time_point m_lastUpdateTime;
};