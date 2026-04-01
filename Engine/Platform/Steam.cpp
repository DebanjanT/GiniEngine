#include "Steam.h"
#include "Core/Logger.h"

// Uncomment when Steamworks SDK is available
// #include <steam/steam_api.h>

namespace Gini {

bool Steam::Init(u32 appId) {
    m_AppId = appId;
    
    // TODO: Initialize Steamworks SDK
    // if (!SteamAPI_Init()) {
    //     GINI_ERROR("Failed to initialize Steam API");
    //     return false;
    // }
    
    m_Initialized = true;
    GINI_INFO("Steam initialized (stub) with AppId: ", appId);
    return true;
}

void Steam::Shutdown() {
    if (!m_Initialized) return;
    
    // SteamAPI_Shutdown();
    
    m_Initialized = false;
    GINI_INFO("Steam shutdown");
}

void Steam::Update() {
    if (!m_Initialized) return;
    
    // SteamAPI_RunCallbacks();
}

u64 Steam::GetSteamId() const {
    if (!m_Initialized) return 0;
    // return SteamUser()->GetSteamID().ConvertToUint64();
    return 76561198000000000ULL; // Stub value
}

std::string Steam::GetPlayerName() const {
    if (!m_Initialized) return "Player";
    // return SteamFriends()->GetPersonaName();
    return "SteamPlayer";
}

SteamUserInfo Steam::GetUserInfo() const {
    SteamUserInfo info;
    info.steamId = GetSteamId();
    info.displayName = GetPlayerName();
    info.online = m_Initialized;
    return info;
}

bool Steam::IsLoggedIn() const {
    if (!m_Initialized) return false;
    // return SteamUser()->BLoggedOn();
    return true;
}

bool Steam::UnlockAchievement(const std::string& achievementId) {
    if (!m_Initialized) return false;
    
    // SteamUserStats()->SetAchievement(achievementId.c_str());
    // SteamUserStats()->StoreStats();
    
    GINI_INFO("Achievement unlocked (stub): ", achievementId);
    return true;
}

bool Steam::SetAchievementProgress(const std::string& achievementId, f32 progress) {
    if (!m_Initialized) return false;
    
    // SteamUserStats()->IndicateAchievementProgress(achievementId.c_str(), 
    //     static_cast<uint32>(progress * 100), 100);
    
    return true;
}

bool Steam::ClearAchievement(const std::string& achievementId) {
    if (!m_Initialized) return false;
    
    // SteamUserStats()->ClearAchievement(achievementId.c_str());
    // SteamUserStats()->StoreStats();
    
    return true;
}

bool Steam::IsAchievementUnlocked(const std::string& achievementId) const {
    if (!m_Initialized) return false;
    
    // bool unlocked = false;
    // SteamUserStats()->GetAchievement(achievementId.c_str(), &unlocked);
    // return unlocked;
    
    return false;
}

std::vector<SteamAchievement> Steam::GetAllAchievements() const {
    std::vector<SteamAchievement> achievements;
    // TODO: Enumerate achievements from Steam
    return achievements;
}

bool Steam::SetStat(const std::string& statName, i32 value) {
    if (!m_Initialized) return false;
    // return SteamUserStats()->SetStat(statName.c_str(), value);
    return true;
}

bool Steam::SetStat(const std::string& statName, f32 value) {
    if (!m_Initialized) return false;
    // return SteamUserStats()->SetStat(statName.c_str(), value);
    return true;
}

i32 Steam::GetStatInt(const std::string& statName) const {
    if (!m_Initialized) return 0;
    // int32 value = 0;
    // SteamUserStats()->GetStat(statName.c_str(), &value);
    // return value;
    return 0;
}

f32 Steam::GetStatFloat(const std::string& statName) const {
    if (!m_Initialized) return 0.0f;
    // float value = 0.0f;
    // SteamUserStats()->GetStat(statName.c_str(), &value);
    // return value;
    return 0.0f;
}

bool Steam::StoreStats() {
    if (!m_Initialized) return false;
    // return SteamUserStats()->StoreStats();
    return true;
}

void Steam::UploadScore(const std::string& leaderboardName, i32 score) {
    if (!m_Initialized) return;
    // TODO: Implement leaderboard score upload
    GINI_INFO("Uploading score (stub): ", leaderboardName, " = ", score);
}

void Steam::DownloadScores(const std::string& leaderboardName, u32 count,
                           std::function<void(const std::vector<SteamLeaderboardEntry>&)> callback) {
    if (!m_Initialized) {
        if (callback) callback({});
        return;
    }
    // TODO: Implement leaderboard download
    if (callback) callback({});
}

bool Steam::SaveToCloud(const std::string& filename, const void* data, u32 size) {
    if (!m_Initialized) return false;
    // return SteamRemoteStorage()->FileWrite(filename.c_str(), data, size);
    GINI_INFO("Cloud save (stub): ", filename);
    return true;
}

bool Steam::LoadFromCloud(const std::string& filename, void* data, u32 maxSize, u32& outSize) {
    if (!m_Initialized) return false;
    // int32 fileSize = SteamRemoteStorage()->GetFileSize(filename.c_str());
    // if (fileSize <= 0 || fileSize > maxSize) return false;
    // outSize = SteamRemoteStorage()->FileRead(filename.c_str(), data, maxSize);
    // return outSize > 0;
    outSize = 0;
    return false;
}

bool Steam::DeleteCloudFile(const std::string& filename) {
    if (!m_Initialized) return false;
    // return SteamRemoteStorage()->FileDelete(filename.c_str());
    return true;
}

bool Steam::IsCloudEnabled() const {
    if (!m_Initialized) return false;
    // return SteamRemoteStorage()->IsCloudEnabledForAccount() && 
    //        SteamRemoteStorage()->IsCloudEnabledForApp();
    return true;
}

std::vector<std::string> Steam::GetCloudFiles() const {
    std::vector<std::string> files;
    // TODO: Enumerate cloud files
    return files;
}

void Steam::CreateLobby(u32 maxMembers, bool isPublic,
                        std::function<void(bool success, u64 lobbyId)> callback) {
    if (!m_Initialized) {
        if (callback) callback(false, 0);
        return;
    }
    // TODO: Implement lobby creation
    GINI_INFO("Creating lobby (stub): max=", maxMembers, " public=", isPublic);
    if (callback) callback(true, 12345);
}

void Steam::JoinLobby(u64 lobbyId, std::function<void(bool success)> callback) {
    if (!m_Initialized) {
        if (callback) callback(false);
        return;
    }
    // TODO: Implement lobby joining
    m_CurrentLobby = lobbyId;
    if (callback) callback(true);
}

void Steam::LeaveLobby() {
    if (!m_Initialized || m_CurrentLobby == 0) return;
    // SteamMatchmaking()->LeaveLobby(CSteamID(m_CurrentLobby));
    m_CurrentLobby = 0;
}

void Steam::SetLobbyData(const std::string& key, const std::string& value) {
    if (!m_Initialized || m_CurrentLobby == 0) return;
    // SteamMatchmaking()->SetLobbyData(CSteamID(m_CurrentLobby), key.c_str(), value.c_str());
}

std::string Steam::GetLobbyData(const std::string& key) const {
    if (!m_Initialized || m_CurrentLobby == 0) return "";
    // return SteamMatchmaking()->GetLobbyData(CSteamID(m_CurrentLobby), key.c_str());
    return "";
}

std::vector<u64> Steam::GetLobbyMembers() const {
    std::vector<u64> members;
    // TODO: Enumerate lobby members
    return members;
}

void Steam::InviteFriend(u64 steamId) {
    if (!m_Initialized || m_CurrentLobby == 0) return;
    // SteamMatchmaking()->InviteUserToLobby(CSteamID(m_CurrentLobby), CSteamID(steamId));
}

void Steam::SearchLobbies(std::function<void(const std::vector<SteamLobbyInfo>&)> callback) {
    if (!m_Initialized) {
        if (callback) callback({});
        return;
    }
    // TODO: Implement lobby search
    if (callback) callback({});
}

std::vector<SteamUserInfo> Steam::GetFriends() const {
    std::vector<SteamUserInfo> friends;
    // TODO: Enumerate friends list
    return friends;
}

bool Steam::InviteFriendToGame(u64 steamId) {
    if (!m_Initialized) return false;
    // return SteamFriends()->InviteUserToGame(CSteamID(steamId), "");
    return true;
}

void Steam::SetRichPresence(const std::string& key, const std::string& value) {
    if (!m_Initialized) return;
    // SteamFriends()->SetRichPresence(key.c_str(), value.c_str());
}

void Steam::ClearRichPresence() {
    if (!m_Initialized) return;
    // SteamFriends()->ClearRichPresence();
}

void Steam::SubscribeToWorkshopItem(u64 itemId) {
    if (!m_Initialized) return;
    // SteamUGC()->SubscribeItem(PublishedFileId_t(itemId));
}

void Steam::UnsubscribeFromWorkshopItem(u64 itemId) {
    if (!m_Initialized) return;
    // SteamUGC()->UnsubscribeItem(PublishedFileId_t(itemId));
}

std::vector<u64> Steam::GetSubscribedWorkshopItems() const {
    std::vector<u64> items;
    // TODO: Enumerate subscribed workshop items
    return items;
}

} // namespace Gini
