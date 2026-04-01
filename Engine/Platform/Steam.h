#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>
#include <functional>

namespace Gini {

// Steam integration stub - requires Steamworks SDK to be linked
// Download from: https://partner.steamgames.com/

struct SteamUserInfo {
    u64 steamId = 0;
    std::string displayName;
    std::string avatarUrl;
    bool online = false;
};

struct SteamAchievement {
    std::string id;
    std::string name;
    std::string description;
    bool unlocked = false;
    f32 progress = 0.0f;
};

struct SteamLeaderboardEntry {
    u64 steamId = 0;
    std::string playerName;
    i32 rank = 0;
    i32 score = 0;
};

struct SteamLobbyInfo {
    u64 lobbyId = 0;
    std::string name;
    u32 memberCount = 0;
    u32 maxMembers = 0;
    bool isPublic = true;
};

class Steam {
public:
    static Steam& Get() {
        static Steam instance;
        return instance;
    }
    
    // Initialization
    bool Init(u32 appId);
    void Shutdown();
    void Update();
    
    bool IsInitialized() const { return m_Initialized; }
    bool IsOverlayActive() const { return m_OverlayActive; }
    
    // User
    u64 GetSteamId() const;
    std::string GetPlayerName() const;
    SteamUserInfo GetUserInfo() const;
    bool IsLoggedIn() const;
    
    // Achievements
    bool UnlockAchievement(const std::string& achievementId);
    bool SetAchievementProgress(const std::string& achievementId, f32 progress);
    bool ClearAchievement(const std::string& achievementId);
    bool IsAchievementUnlocked(const std::string& achievementId) const;
    std::vector<SteamAchievement> GetAllAchievements() const;
    
    // Stats
    bool SetStat(const std::string& statName, i32 value);
    bool SetStat(const std::string& statName, f32 value);
    i32 GetStatInt(const std::string& statName) const;
    f32 GetStatFloat(const std::string& statName) const;
    bool StoreStats();
    
    // Leaderboards
    void UploadScore(const std::string& leaderboardName, i32 score);
    void DownloadScores(const std::string& leaderboardName, u32 count, 
                        std::function<void(const std::vector<SteamLeaderboardEntry>&)> callback);
    
    // Cloud Save
    bool SaveToCloud(const std::string& filename, const void* data, u32 size);
    bool LoadFromCloud(const std::string& filename, void* data, u32 maxSize, u32& outSize);
    bool DeleteCloudFile(const std::string& filename);
    bool IsCloudEnabled() const;
    std::vector<std::string> GetCloudFiles() const;
    
    // Matchmaking / Lobbies
    void CreateLobby(u32 maxMembers, bool isPublic, 
                     std::function<void(bool success, u64 lobbyId)> callback);
    void JoinLobby(u64 lobbyId, std::function<void(bool success)> callback);
    void LeaveLobby();
    void SetLobbyData(const std::string& key, const std::string& value);
    std::string GetLobbyData(const std::string& key) const;
    std::vector<u64> GetLobbyMembers() const;
    void InviteFriend(u64 steamId);
    void SearchLobbies(std::function<void(const std::vector<SteamLobbyInfo>&)> callback);
    
    // Friends
    std::vector<SteamUserInfo> GetFriends() const;
    bool InviteFriendToGame(u64 steamId);
    
    // Rich Presence
    void SetRichPresence(const std::string& key, const std::string& value);
    void ClearRichPresence();
    
    // Workshop
    void SubscribeToWorkshopItem(u64 itemId);
    void UnsubscribeFromWorkshopItem(u64 itemId);
    std::vector<u64> GetSubscribedWorkshopItems() const;
    
    // Callbacks
    using OverlayCallback = std::function<void(bool active)>;
    using LobbyJoinCallback = std::function<void(u64 lobbyId, u64 steamId)>;
    using GameInviteCallback = std::function<void(u64 fromSteamId, const std::string& connectString)>;
    
    void SetOverlayCallback(OverlayCallback callback) { m_OverlayCallback = callback; }
    void SetLobbyJoinCallback(LobbyJoinCallback callback) { m_LobbyJoinCallback = callback; }
    void SetGameInviteCallback(GameInviteCallback callback) { m_GameInviteCallback = callback; }
    
private:
    Steam() = default;
    ~Steam() = default;
    
    bool m_Initialized = false;
    bool m_OverlayActive = false;
    u32 m_AppId = 0;
    u64 m_CurrentLobby = 0;
    
    OverlayCallback m_OverlayCallback;
    LobbyJoinCallback m_LobbyJoinCallback;
    GameInviteCallback m_GameInviteCallback;
};

// Macro to check if Steam is available
#define STEAM_AVAILABLE() (Steam::Get().IsInitialized())

} // namespace Gini
