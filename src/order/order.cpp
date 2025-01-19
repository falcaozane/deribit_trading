#include "order/order.hpp"
#include <stdexcept>

Order::Order(const std::string& instrument, OrderSide side, OrderType type, 
             double price, double amount)
    : m_instrument(instrument)
    , m_side(side)
    , m_type(type)
    , m_price(price)
    , m_amount(amount)
    , m_filledAmount(0.0)
    , m_status(OrderStatus::PENDING)
    , m_creationTime(std::chrono::system_clock::now())
    , m_lastUpdateTime(m_creationTime)
{
    if (price < 0.0 || amount <= 0.0) {
        throw std::invalid_argument("Invalid price or amount");
    }
}

void Order::setStatus(OrderStatus status) {
    m_status = status;
    updateLastUpdateTime();
}

void Order::setFilledAmount(double amount) {
    if (amount < 0.0 || amount > m_amount) {
        throw std::invalid_argument("Invalid filled amount");
    }
    
    m_filledAmount = amount;
    if (m_filledAmount == m_amount) {
        setStatus(OrderStatus::FILLED);
    } else if (m_filledAmount > 0) {
        setStatus(OrderStatus::PARTIALLY_FILLED);
    }
    
    updateLastUpdateTime();
}

void Order::setPrice(double price) {
    if (price < 0.0) {
        throw std::invalid_argument("Invalid price");
    }
    m_price = price;
    updateLastUpdateTime();
}

void Order::setAmount(double amount) {
    if (amount <= 0.0) {
        throw std::invalid_argument("Invalid amount");
    }
    if (amount < m_filledAmount) {
        throw std::invalid_argument("New amount cannot be less than filled amount");
    }
    m_amount = amount;
    updateLastUpdateTime();
}

bool Order::isActive() const {
    return m_status == OrderStatus::PENDING ||
           m_status == OrderStatus::OPEN ||
           m_status == OrderStatus::PARTIALLY_FILLED;
}