// src/main.cpp
#include <iostream>
#include "api/client.hpp"
#include "utils/logger.hpp"

int main() {
    try {
        std::cout << "Deribit Trading System Starting..." << std::endl;
        // Main program logic will go here
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}