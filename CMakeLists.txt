cmake_minimum_required(VERSION 3.10)
project(deribit_trading VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add source files
add_executable(deribit_trading 
    src/main.cpp
    src/api/client.cpp
    src/api/websocket.cpp
    src/order/order.cpp
    src/order/orderbook.cpp
    src/market/market_data.cpp
    src/utils/logger.cpp
    src/utils/config.cpp
    src/utils/utils.cpp
)

# Include directories
target_include_directories(deribit_trading PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(deribit_trading PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    nlohmann_json::nlohmann_json
)