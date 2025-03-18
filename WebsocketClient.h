#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <iostream>
#include <queue>
#include <thread>

class WebSocketClient {
public:
    WebSocketClient(const std::string& uri);
    ~WebSocketClient();

    void sendMessage(const std::string& message);
    void run();
    void stop();

    std::string getMsgFromQueue() { 
        if (!msgQueue.empty()) {
            std::string msg = msgQueue.front();
            msgQueue.pop();
            return msg;
        }
        else return "";
    }

private:


    ix::WebSocket webSocket;
    std::string uri;
    std::queue<std::string> msgQueue;
    void onMessage(const ix::WebSocketMessagePtr& msg);
};