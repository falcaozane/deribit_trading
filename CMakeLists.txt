cmake_minimum_required(VERSION 3.10)
project(DeribitTrading VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add dependencies (we'll need these later)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
# You'll need to add websocketpp and json libraries later

# Add source files
add_executable(deribit_trading 
    src/main.cpp
    src/api/client.cpp
    src/api/websocket.cpp
    src/order/order.cpp
    src/order/orderbook.cpp
    src/market/market_data.cpp
    src/utils/logger.cpp
)

# Include directories
target_include_directories(deribit_trading PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(deribit_trading PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
)