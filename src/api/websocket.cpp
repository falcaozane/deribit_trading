#include "api/websocket.hpp"
#include <iostream>

DeribitWebSocket::DeribitWebSocket() : m_connected(false) {
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.clear_error_channels(websocketpp::log::elevel::all);

    m_client.init_asio();
    m_client.set_tls_init_handler([](websocketpp::connection_hdl) {
        return std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    });

    m_client.set_message_handler([this](auto hdl, auto msg) {
        this->onMessage(hdl, msg);
    });
    m_client.set_open_handler([this](auto hdl) {
        this->onOpen(hdl);
    });
    m_client.set_close_handler([this](auto hdl) {
        this->onClose(hdl);
    });
    m_client.set_fail_handler([this](auto hdl) {
        this->onFail(hdl);
    });
}

DeribitWebSocket::~DeribitWebSocket() {
    close();
}

void DeribitWebSocket::connect(const std::string& uri) {
    websocketpp::lib::error_code ec;
    WebsocketClient::connection_ptr con = m_client.get_connection(uri, ec);
    
    if (ec) {
        throw std::runtime_error("Could not create connection: " + ec.message());
    }

    m_client.connect(con);
    m_client.run();
}

void DeribitWebSocket::subscribe(const std::string& channel) {
    if (!m_connected) {
        throw std::runtime_error("WebSocket not connected");
    }

    nlohmann::json msg = {
        {"jsonrpc", "2.0"},
        {"method", "public/subscribe"},
        {"params", {
            {"channels", {channel}}
        }},
        {"id", 42}
    };

    m_client.send(m_hdl, msg.dump(), websocketpp::frame::opcode::text);
    m_subscriptions[channel] = true;
}

void DeribitWebSocket::unsubscribe(const std::string& channel) {
    if (!m_connected) {
        throw std::runtime_error("WebSocket not connected");
    }

    nlohmann::json msg = {
        {"jsonrpc", "2.0"},
        {"method", "public/unsubscribe"},
        {"params", {
            {"channels", {channel}}
        }},
        {"id", 42}
    };

    m_client.send(m_hdl, msg.dump(), websocketpp::frame::opcode::text);
    m_subscriptions.erase(channel);
}

void DeribitWebSocket::setMessageCallback(MessageCallback callback) {
    m_messageCallback = callback;
}

bool DeribitWebSocket::isConnected() const {
    return m_connected;
}

void DeribitWebSocket::close() {
    if (m_connected) {
        m_client.close(m_hdl, websocketpp::close::status::normal, "");
        m_connected = false;
    }
}

void DeribitWebSocket::onMessage(websocketpp::connection_hdl hdl, WebsocketClient::message_ptr msg) {
    if (m_messageCallback) {
        m_messageCallback(msg->get_payload());
    }
}

void DeribitWebSocket::onOpen(websocketpp::connection_hdl hdl) {
    m_hdl = hdl;
    m_connected = true;
}

void DeribitWebSocket::onClose(websocketpp::connection_hdl hdl) {
    m_connected = false;
}

void DeribitWebSocket::onFail(websocketpp::connection_hdl hdl) {
    m_connected = false;
}