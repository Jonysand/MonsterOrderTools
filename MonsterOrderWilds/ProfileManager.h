#pragma once
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

struct KeywordRecord {
    int32_t id = 0;
    std::string word;
    int32_t frequency = 0;
    int64_t lastSeenTimestamp = 0;
};

struct UserProfileData {
    uint64_t uid = 0;
    std::string username;
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
    std::vector<KeywordRecord> keywords;
    std::vector<std::pair<int64_t, std::string>> danmuHistory;
};

class ProfileManager {
    DECLARE_SINGLETON(ProfileManager)

public:
    bool Init();

    void SaveProfile(const UserProfileData& profile);
    bool LoadProfile(uint64_t uid, UserProfileData& outProfile);
    void DeleteProfile(uint64_t uid);

    void RecordCheckin(uint64_t uid, const std::string& username, int32_t checkinDate);
    int32_t CalculateContinuousDays(uint64_t uid, int32_t checkinDate);

    void AddKeyword(uint64_t uid, const std::string& keyword);
    void AddDanmuHistory(uint64_t uid, int64_t timestamp, const std::string& content);

    std::string SerializeToJson(const UserProfileData& profile);
    bool DeserializeFromJson(const std::string& json, UserProfileData& outProfile);

private:
    std::string GetDbPath() const;
    std::string KeywordsToJson(const std::vector<KeywordRecord>& keywords) const;
    std::vector<KeywordRecord> JsonToKeywords(const std::string& json) const;
    std::string DanmuHistoryToJson(const std::vector<std::pair<int64_t, std::string>>& history) const;
    std::vector<std::pair<int64_t, std::string>> JsonToDanmuHistory(const std::string& json) const;
    bool LoadProfileFromDb(uint64_t uid, UserProfileData& outProfile);
    void SaveProfileToDb(const UserProfileData& profile);

    std::string dbPath_;
    void* storage_ = nullptr;
    std::map<uint64_t, UserProfileData> profiles_;
    std::mutex profilesLock_;
};
