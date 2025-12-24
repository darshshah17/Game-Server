#include "PlayerManager.h"
#include <algorithm>

PlayerManager::PlayerManager() : m_nextPlayerId(1) {
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::addPlayer(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Player player;
    player.id = playerId;
    player.username = "Player" + std::to_string(playerId);
    player.inMatch = false;
    player.currentMatchId = "";
    player.lastPingTime = 0;
    player.latency = 0.0f;
    
    m_players[playerId] = player;
}

void PlayerManager::removePlayer(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_players.erase(playerId);
}

bool PlayerManager::playerExists(uint64_t playerId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_players.find(playerId) != m_players.end();
}

Player* PlayerManager::getPlayer(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        return &it->second;
    }
    return nullptr;
}

const Player* PlayerManager::getPlayer(uint64_t playerId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        return &it->second;
    }
    return nullptr;
}

void PlayerManager::setPlayerUsername(uint64_t playerId, const std::string& username) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.username = username;
    }
}

void PlayerManager::setPlayerInMatch(uint64_t playerId, bool inMatch, const std::string& matchId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.inMatch = inMatch;
        it->second.currentMatchId = matchId;
    }
}

void PlayerManager::updatePlayerLatency(uint64_t playerId, float latency) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.latency = latency;
    }
}

void PlayerManager::updatePlayerPing(uint64_t playerId, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.lastPingTime = timestamp;
    }
}

size_t PlayerManager::getPlayerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_players.size();
}

std::vector<uint64_t> PlayerManager::getAllPlayerIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint64_t> ids;
    ids.reserve(m_players.size());
    for (const auto& pair : m_players) {
        ids.push_back(pair.first);
    }
    return ids;
}

