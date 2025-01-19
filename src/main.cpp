#include "api/client.hpp"
#include "api/websocket.hpp"
#include "order/order.hpp"
#include "order/orderbook.hpp"
#include "market/market_data.hpp"
#include "utils/logger.hpp"
#include "utils/config.hpp"
#include "types.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

// Add using directives for convenience
using namespace deribit;

std::atomic<bool> running{true};

void signalHandler(int signal) {
    running = false;
    std::cout << "\nShutting down..." << std::endl;
}

// Callback for order book updates
void handleOrderBookUpdate(const std::string& instrument, 
                         const std::string& channel,
                         const nlohmann::json& data) {
    auto& logger = Logger::getInstance();
    logger.info("OrderBook Update - Instrument: ", instrument, 
                ", Channel: ", channel);
}

// Callback for market data updates
void handleMarketData(const std::string& instrument, 
                    const std::string& channel,
                    const nlohmann::json& data) {
    auto& logger = Logger::getInstance();
    if (channel.find("trades") != std::string::npos) {
        logger.info("Trade Update - Instrument: ", instrument,
                   ", Data: ", data.dump());
    } else if (channel.find("ticker") != std::string::npos) {
        logger.info("Ticker Update - Instrument: ", instrument,
                   ", Data: ", data.dump());
    }
}

// Callback for order updates
void handleOrderUpdate(const Order& order) {
    auto& logger = Logger::getInstance();
    logger.info("Order Update - ID: ", order.getOrderId(),
                ", Status: ", static_cast<int>(order.getStatus()));
}

int main(int argc, char* argv[]) {
    try {
        // Set up signal handling
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // Initialize logger
        auto& logger = Logger::getInstance();
        logger.setLogFile("trading_system.log");
        logger.setLogLevel(LogLevel::INFO);

        // Load configuration
        auto& config = Config::getInstance();
        if (argc > 1) {
            config.loadFromFile(argv[1]);
        }

        // Initialize API client
        std::string apiKey = config.getApiKey();
        std::string apiSecret = config.getApiSecret();
        DeribitClient client(apiKey, apiSecret);

        // Initialize WebSocket connection
        DeribitWebSocket ws;
        ws.setMessageCallback([&logger](const std::string& msg) {
            logger.debug("WebSocket message received: ", msg);
        });

        // Connect to WebSocket
        logger.info("Connecting to WebSocket...");
        ws.connect(config.getWsUrl());

        // Initialize Market Data Manager
        MarketDataManager marketData(config.getWsUrl());
        marketData.setOrderBookCallback(handleOrderBookUpdate);
        marketData.setMarketDataCallback(handleMarketData);

        // Subscribe to instruments
        const std::vector<std::string> instruments = {
            "BTC-PERPETUAL",
            "ETH-PERPETUAL"
        };

        for (const auto& instrument : instruments) {
            marketData.subscribeToOrderBook(instrument);
            marketData.subscribe(instrument, true, true, true);  // orderbook, trades, ticker
            logger.info("Subscribed to market data for ", instrument);
        }

        // Authenticate
        if (!client.authenticate()) {
            logger.error("Authentication failed");
            return 1;
        }
        logger.info("Authentication successful");

        // Main loop
        logger.info("Starting main loop...");
        while (running) {
            try {
                // Process any pending tasks
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Monitor positions
                for (const auto& instrument : instruments) {
                    auto position = client.getPositions(instrument);
                    logger.debug("Position for ", instrument, ": ", position.dump());
                }

            } catch (const std::exception& e) {
                logger.error("Error in main loop: ", e.what());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

        // Clean shutdown
        logger.info("Shutting down...");
        
        // Unsubscribe from market data
        for (const auto& instrument : instruments) {
            marketData.unsubscribeFromOrderBook(instrument);
            logger.info("Unsubscribed from market data for ", instrument);
        }

        // Close WebSocket connection
        ws.close();
        logger.info("WebSocket connection closed");

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}