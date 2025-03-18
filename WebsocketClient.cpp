#include "WebSocketClient.h"

WebSocketClient::WebSocketClient(const std::string& uri) : uri(uri) {
    webSocket.setUrl(uri);
    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        onMessage(msg);
        });
}

WebSocketClient::~WebSocketClient() {
    stop();
}

void WebSocketClient::onMessage(const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Message) { 
        //std::cout << "Received: " << msg->str << std::endl; 
        msgQueue.push(msg->str);
    }
    else if (msg->type == ix::WebSocketMessageType::Open) { 
        std::cout << "Connected to WebSocket server!" << std::endl; 
    }
    else if (msg->type == ix::WebSocketMessageType::Close) { 
        std::cout << "Disconnected from WebSocket server.Code: " << msg->closeInfo.code 
            << ", Reason: " << msg->closeInfo.reason << std::endl; 
    }
    else if (msg->type == ix::WebSocketMessageType::Error) { 
        std::cerr << "WebSocket Error: " << msg->errorInfo.reason << std::endl; 
    }
}

void WebSocketClient::sendMessage(const std::string& message) {
    webSocket.sendText(message);
}

void WebSocketClient::run() {
    std::cout << "connect\n";
    webSocket.start();

}

void WebSocketClient::stop() {
    webSocket.stop();
}