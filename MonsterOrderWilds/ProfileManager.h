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
    std::string uid;
    std::string username;
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
    std::vector<KeywordRecord> keywords;
    std::vector<std::pair<int64_t, std::string>> danmuHistory;
};

struct DailyLikeData {
    std::string uid;
    int32_t likeDate = 0;
    int32_t totalLikes = 0;
};

struct LikeStreakData {
    std::string uid;
    int32_t currentStreak = 0;
    int32_t lastLikeDate = 0;
    int32_t streakRewardIssued = 0;
};

struct RetroactiveCardData {
    std::string uid;
    int32_t cardCount = 0;
    int32_t totalEarned = 0;
    int32_t monthlyFirstClaimed = 0;
    int32_t lastEarnedDate = 0;
};

class ProfileManager {
    DECLARE_SINGLETON(ProfileManager)

public:
    bool Init();

    void SaveProfile(const UserProfileData& profile);
    bool LoadProfile(const std::string& uid, UserProfileData& outProfile);
    void DeleteProfile(const std::string& uid);

    void RecordCheckin(const std::string& uid, const std::string& username, int32_t checkinDate);
    void RecordCheckinAsync(const std::string& uid, const std::string& username, int32_t checkinDate, int32_t continuousDays);
    int32_t CalculateContinuousDays(const std::string& uid, int32_t checkinDate);

    void AddKeyword(const std::string& uid, const std::string& keyword);
    void AddDanmuHistory(const std::string& uid, int64_t timestamp, const std::string& content);

    std::string SerializeToJson(const UserProfileData& profile);
    bool DeserializeFromJson(const std::string& json, UserProfileData& outProfile);

    bool AddDailyLike(const std::string& uid, int32_t likeDate, int32_t likeCount, int32_t& outTotalLikes);
    bool GetDailyLike(const std::string& uid, int32_t likeDate, DailyLikeData& outData);

    bool LoadLikeStreak(const std::string& uid, LikeStreakData& outData);
    bool SaveLikeStreak(const LikeStreakData& data);

    bool LoadRetroactiveCards(const std::string& uid, RetroactiveCardData& outData);
    bool SaveRetroactiveCards(const RetroactiveCardData& data);
    bool DeductRetroactiveCard(const std::string& uid);
    bool AddRetroactiveCard(const std::string& uid, int32_t count = 1);

    // 原子性奖励发放接口（事务保护，防止崩溃导致重复奖励）
    bool IssueStreakReward(const std::string& uid, int32_t date);
    bool IssueMonthlyFirstReward(const std::string& uid, int32_t date);

    bool InsertRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t checkinDate);
    int32_t FindLastMissingCheckinDate(const std::string& uid, int32_t currentDate);

    // 原子性执行补签操作：扣卡 + 插记录 + 更新 profile，全部在一个事务中
    bool ExecuteRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t targetDate, int32_t& outNewCardCount);

    // 获取原始数据库句柄（用于测试清理等场景）
    void* GetStorage() const { return storage_; }

private:
    std::string GetDbPath() const;
    std::string KeywordsToJson(const std::vector<KeywordRecord>& keywords) const;
    std::vector<KeywordRecord> JsonToKeywords(const std::string& json) const;
    std::string DanmuHistoryToJson(const std::vector<std::pair<int64_t, std::string>>& history) const;
    std::vector<std::pair<int64_t, std::string>> JsonToDanmuHistory(const std::string& json) const;
    bool LoadProfileFromDb(const std::string& uid, UserProfileData& outProfile);
    void SaveProfileToDb(const UserProfileData& profile);
    bool GetLastCheckinRecordFromDb(const std::string& uid, int32_t& outLastDate, int32_t& outContinuousDays);
    void EvictOldestProfileIfNeeded();

    std::string dbPath_;
    void* storage_ = nullptr;
    std::map<std::string, UserProfileData> profiles_;
    std::recursive_mutex profilesLock_;
};
