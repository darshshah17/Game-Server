#include "GameServer.h"
#include "WebSocketServer.h"
#include "GameStateManager.h"
#include "MatchmakingSystem.h"
#include "ChatSystem.h"
#include "PlayerManager.h"
#include <iostream>
#include <chrono>
#include <json/json.h>
#include <thread>

GameServer::GameServer(int port) 
    : m_running(false) {
    m_playerManager = std::make_unique<PlayerManager>();
    m_wsServer = std::make_unique<WebSocketServer>(port);
    
    // Pass WebSocketServer to components that need it
    m_matchmakingSystem = std::make_unique<MatchmakingSystem>(m_playerManager.get(), m_wsServer.get());
    m_chatSystem = std::make_unique<ChatSystem>(m_playerManager.get(), m_wsServer.get());
    m_gameStateManager = std::make_unique<GameStateManager>(m_playerManager.get(), m_wsServer.get());
    
    m_wsServer->setOnConnect([this](uint64_t id) { onPlayerConnected(id); });
    m_wsServer->setOnDisconnect([this](uint64_t id) { onPlayerDisconnected(id); });
    m_wsServer->setOnMessage([this](uint64_t id, const std::string& msg) { handleMessage(id, msg); });
}

GameServer::~GameServer() {
    stop();
}

void GameServer::run() {
    m_running = true;
    m_gameLoopThread = std::thread(&GameServer::gameLoop, this);
    m_wsServer->run();
}

void GameServer::stop() {
    if (m_running) {
        m_running = false;
        m_wsServer->stop();
        if (m_gameLoopThread.joinable()) {
            m_gameLoopThread.join();
        }
    }
}

void GameServer::gameLoop() {
    const int TICK_RATE = 120; // Increased to 120 ticks per second for lower latency
    const auto TICK_DURATION = std::chrono::microseconds(1000000 / TICK_RATE);
    
    while (m_running) {
        auto start = std::chrono::steady_clock::now();
        
        // Update game state
        m_gameStateManager->tick();
        
        // Process matchmaking
        m_matchmakingSystem->process();
        
        // Send state updates to clients (handled inside tick() with dirty check)
        // m_gameStateManager->broadcastStateUpdates();
        
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto sleepTime = TICK_DURATION - elapsed;
        
        if (sleepTime.count() > 0) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void GameServer::onPlayerConnected(uint64_t playerId) {
    std::cout << "[GameServer] Player " << playerId << " connected" << std::endl;
    m_playerManager->addPlayer(playerId);
    
    Json::Value response;
    response["type"] = "connected";
    response["playerId"] = static_cast<Json::UInt64>(playerId);
    response["serverTime"] = static_cast<Json::UInt64>(m_gameStateManager->getServerTime());
    
    m_wsServer->send(playerId, response.toStyledString());
}

void GameServer::onPlayerDisconnected(uint64_t playerId) {
    std::cout << "Player " << playerId << " disconnected" << std::endl;
    m_gameStateManager->removePlayer(playerId);
    m_matchmakingSystem->removePlayer(playerId);
    m_chatSystem->removePlayer(playerId);
    m_playerManager->removePlayer(playerId);
}

void GameServer::handleMessage(uint64_t playerId, const std::string& message) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(message, root)) {
        std::cerr << "Failed to parse message from player " << playerId << std::endl;
        return;
    }
    
    std::string type = root["type"].asString();
    
    if (type == "matchmaking_request") {
        std::cout << "[Server] Received matchmaking request from player " << playerId << std::endl;
        m_matchmakingSystem->queuePlayer(playerId, root);
    }
    else if (type == "chat_message") {
        m_chatSystem->handleMessage(playerId, root);
    }
    else if (type == "game_action") {
        m_gameStateManager->handlePlayerAction(playerId, root);
    }
    else if (type == "ping") {
        Json::Value response;
        response["type"] = "pong";
        response["serverTime"] = static_cast<Json::UInt64>(m_gameStateManager->getServerTime());
        m_wsServer->send(playerId, response.toStyledString());
    }
    else {
        std::cerr << "Unknown message type: " << type << std::endl;
    }
}

