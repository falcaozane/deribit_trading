#pragma once
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>
#include <map>

using WebsocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using MessageCallback = std::function<void(const std::string&)>;

class DeribitWebSocket {
public:
    DeribitWebSocket();
    ~DeribitWebSocket();

    void connect(const std::string& uri);
    void subscribe(const std::string& channel);
    void unsubscribe(const std::string& channel);
    void setMessageCallback(MessageCallback callback);
    bool isConnected() const;
    void close();

private:
    void onMessage(websocketpp::connection_hdl hdl, WebsocketClient::message_ptr msg);
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);

    WebsocketClient m_client;
    websocketpp::connection_hdl m_hdl;
    MessageCallback m_messageCallback;
    bool m_connected;
    std::map<std::string, bool> m_subscriptions;
};