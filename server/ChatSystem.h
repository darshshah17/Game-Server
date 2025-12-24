#pragma once

#include "PlayerManager.h"
#include <json/json.h>
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

class WebSocketServer;

struct ChatMessage {
    uint64_t playerId;
    std::string username;
    std::string message;
    uint64_t timestamp;
    std::string channel; // "global", "match", etc.
};

class ChatSystem {
public:
    ChatSystem(PlayerManager* playerManager, WebSocketServer* wsServer);
    ~ChatSystem();
    
    void handleMessage(uint64_t playerId, const Json::Value& messageData); // JSON variant
    void removePlayer(uint64_t playerId);
    
    void sendMessage(uint64_t playerId, const std::string& message, const std::string& channel = "global");
    std::vector<ChatMessage> getRecentMessages(const std::string& channel = "global", int count = 50) const;
    
private:
    PlayerManager* m_playerManager;
    WebSocketServer* m_wsServer;
    std::vector<ChatMessage> m_globalMessages;
    std::mutex m_messagesMutex;
    
    static const size_t MAX_MESSAGES_PER_CHANNEL = 1000;
    
    void broadcastMessage(const ChatMessage& chatMsg);
    bool validateMessage(const std::string& message) const;
};

