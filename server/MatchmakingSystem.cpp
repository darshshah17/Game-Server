#include "MatchmakingSystem.h"
#include "WebSocketServer.h"
#include <json/json.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>

MatchmakingSystem::MatchmakingSystem(PlayerManager* playerManager, WebSocketServer* wsServer) 
    : m_playerManager(playerManager), m_wsServer(wsServer) {
}

MatchmakingSystem::~MatchmakingSystem() {
}

void MatchmakingSystem::queuePlayer(uint64_t playerId, const std::string& gameMode, int minPlayers, int maxPlayers) {
    MatchmakingRequest request;
    request.playerId = playerId;
    request.gameMode = gameMode;
    request.minPlayers = minPlayers;
    request.maxPlayers = maxPlayers;
    request.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push(request);
    std::cout << "[Matchmaking] Player " << playerId << " queued for " << gameMode 
              << " (min: " << minPlayers << ", max: " << maxPlayers << ")" << std::endl;
    std::cout << "[Matchmaking] Queue size: " << m_queue.size() << std::endl;
}

void MatchmakingSystem::queuePlayer(uint64_t playerId, const Json::Value& requestData) {
    std::string gameMode = requestData.get("gameMode", "default").asString();
    int minPlayers = requestData.get("minPlayers", 2).asInt();
    int maxPlayers = requestData.get("maxPlayers", 4).asInt();
    
    queuePlayer(playerId, gameMode, minPlayers, maxPlayers);
}

void MatchmakingSystem::removePlayer(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Remove from queue
    std::queue<MatchmakingRequest> newQueue;
    while (!m_queue.empty()) {
        if (m_queue.front().playerId != playerId) {
            newQueue.push(m_queue.front());
        }
        m_queue.pop();
    }
    m_queue = newQueue;
    
    // Remove from match
    std::lock_guard<std::mutex> matchLock(m_matchesMutex);
    auto it = m_playerToMatch.find(playerId);
    if (it != m_playerToMatch.end()) {
        std::string matchId = it->second;
        m_playerToMatch.erase(it);
        
        auto matchIt = m_matches.find(matchId);
        if (matchIt != m_matches.end()) {
            auto& players = matchIt->second.players;
            players.erase(std::remove(players.begin(), players.end(), playerId), players.end());
            
            if (players.empty()) {
                m_matches.erase(matchIt);
            }
        }
    }
}

void MatchmakingSystem::process() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    if (m_queue.size() < 2) {
        return; // Need at least 2 players
    }
    
    std::cout << "[Matchmaking] Processing queue with " << m_queue.size() << " players" << std::endl;
    
    // Group players by game mode
    std::unordered_map<std::string, std::vector<MatchmakingRequest>> byGameMode;
    
    std::queue<MatchmakingRequest> tempQueue = m_queue;
    while (!tempQueue.empty()) {
        auto request = tempQueue.front();
        tempQueue.pop();
        
        if (m_playerManager->playerExists(request.playerId)) {
            byGameMode[request.gameMode].push_back(request);
        }
    }
    
    // Try to form matches for each game mode
    for (auto& pair : byGameMode) {
        auto& requests = pair.second;
        
        while (requests.size() >= 2) {
            // Find a group that can form a match
            std::vector<MatchmakingRequest> matchGroup;
            std::string gameMode = pair.first;
            
            int minPlayers = requests[0].minPlayers;
            int maxPlayers = requests[0].maxPlayers;
            
            // Take up to maxPlayers from the queue
            for (size_t i = 0; i < requests.size() && matchGroup.size() < static_cast<size_t>(maxPlayers); ++i) {
                if (requests[i].gameMode == gameMode) {
                    matchGroup.push_back(requests[i]);
                }
            }
            
            if (matchGroup.size() >= static_cast<size_t>(minPlayers)) {
                // Create match
                std::vector<uint64_t> playerIds;
                for (const auto& req : matchGroup) {
                    playerIds.push_back(req.playerId);
                }
                
                std::cout << "[Matchmaking] Creating match with " << playerIds.size() 
                          << " players for game mode: " << gameMode << std::endl;
                createMatch(playerIds, gameMode);
                
                // Remove matched players from queue
                for (const auto& req : matchGroup) {
                    requests.erase(
                        std::remove_if(requests.begin(), requests.end(),
                            [&req](const MatchmakingRequest& r) { return r.playerId == req.playerId; }),
                        requests.end());
                }
                
                // Remove from main queue
                std::queue<MatchmakingRequest> newQueue;
                while (!m_queue.empty()) {
                    bool shouldRemove = false;
                    for (const auto& req : matchGroup) {
                        if (m_queue.front().playerId == req.playerId) {
                            shouldRemove = true;
                            break;
                        }
                    }
                    if (!shouldRemove) {
                        newQueue.push(m_queue.front());
                    }
                    m_queue.pop();
                }
                m_queue = newQueue;
            } else {
                break; // Not enough players for this game mode
            }
        }
    }
}

std::string MatchmakingSystem::generateMatchId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void MatchmakingSystem::createMatch(const std::vector<uint64_t>& players, const std::string& gameMode) {
    std::lock_guard<std::mutex> lock(m_matchesMutex);
    
    Match match;
    match.matchId = generateMatchId();
    match.players = players;
    match.gameMode = gameMode;
    match.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    match.isActive = true;
    
    m_matches[match.matchId] = match;
    
    for (uint64_t playerId : players) {
        m_playerToMatch[playerId] = match.matchId;
        m_playerManager->setPlayerInMatch(playerId, true, match.matchId);
    }
    
    notifyMatchCreated(match);
}

void MatchmakingSystem::notifyMatchCreated(const Match& match) {
    Json::Value notification;
    notification["type"] = "match_found";
    notification["matchId"] = match.matchId;
    notification["gameMode"] = match.gameMode;
    
    Json::Value playersJson(Json::arrayValue);
    for (uint64_t playerId : match.players) {
        playersJson.append(static_cast<Json::UInt64>(playerId));
    }
    notification["players"] = playersJson;
    
    std::string message = notification.toStyledString();
    
    if (m_wsServer) {
        // Send to all players in the match
        for (uint64_t playerId : match.players) {
            m_wsServer->send(playerId, message);
            m_wsServer->setClientRoom(playerId, match.matchId);
        }
    }
}

Match* MatchmakingSystem::getMatch(const std::string& matchId) {
    std::lock_guard<std::mutex> lock(m_matchesMutex);
    auto it = m_matches.find(matchId);
    if (it != m_matches.end()) {
        return &it->second;
    }
    return nullptr;
}

Match* MatchmakingSystem::getPlayerMatch(uint64_t playerId) {
    std::lock_guard<std::mutex> lock(m_matchesMutex);
    auto it = m_playerToMatch.find(playerId);
    if (it != m_playerToMatch.end()) {
        return getMatch(it->second);
    }
    return nullptr;
}

void MatchmakingSystem::endMatch(const std::string& matchId) {
    std::lock_guard<std::mutex> lock(m_matchesMutex);
    auto it = m_matches.find(matchId);
    if (it != m_matches.end()) {
        for (uint64_t playerId : it->second.players) {
            m_playerToMatch.erase(playerId);
            m_playerManager->setPlayerInMatch(playerId, false, "");
        }
        m_matches.erase(it);
    }
}

bool MatchmakingSystem::canFormMatch(const MatchmakingRequest& request, const std::vector<MatchmakingRequest>& candidates) {
    // Check if we can form a match with the given candidates
    int count = 1; // Include the request itself
    for (const auto& candidate : candidates) {
        if (candidate.gameMode == request.gameMode) {
            count++;
        }
    }
    return count >= request.minPlayers && count <= request.maxPlayers;
}

