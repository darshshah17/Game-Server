#pragma once

#include <memory>
#include <thread>
#include <atomic>

// Forward declarations to avoid circular dependencies
class WebSocketServer;
class GameStateManager;
class MatchmakingSystem;
class ChatSystem;
class PlayerManager;

class GameServer {
public:
    GameServer(int port);
    ~GameServer();
    
    void run();
    void stop();
    
private:
    std::unique_ptr<WebSocketServer> m_wsServer;
    std::unique_ptr<GameStateManager> m_gameStateManager;
    std::unique_ptr<MatchmakingSystem> m_matchmakingSystem;
    std::unique_ptr<ChatSystem> m_chatSystem;
    std::unique_ptr<PlayerManager> m_playerManager;
    
    std::thread m_gameLoopThread;
    std::atomic<bool> m_running;
    
    void gameLoop();
    void handleMessage(uint64_t playerId, const std::string& message);
    void onPlayerConnected(uint64_t playerId);
    void onPlayerDisconnected(uint64_t playerId);
};
