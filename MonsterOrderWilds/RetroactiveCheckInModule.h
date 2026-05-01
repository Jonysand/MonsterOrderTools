#pragma once
#include "framework.h"
#include "LikeEvent.h"
#include "DanmuProcessor.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <array>
#include <functional>

class ProfileManager;

class RetroactiveCheckInModule {
    DECLARE_SINGLETON(RetroactiveCheckInModule)

public:
    bool Init();

    void PushLikeEvent(const LikeEvent& event);

    void PushDanmuEvent(const DanmuProcessor::CaptainDanmuEvent& event);

    void SetTriggerWords(const std::string& words);

    bool IsRetroactiveMessage(const std::string& content) const;
    bool IsQueryMessage(const std::string& content) const;

private:
    void ProcessLike(const LikeEvent& event);

    bool CheckRule1_StreakReward(const std::string& uid, int32_t date);
    bool CheckRule2_MonthlyFirst(const std::string& uid, int32_t date, int32_t totalLikes);

    void HandleRetroactiveCommand(const DanmuProcessor::CaptainDanmuEvent& event);
    void HandleQueryCommand(const DanmuProcessor::CaptainDanmuEvent& event);

    bool ExecuteRetroactive(const std::string& uid, const std::string& username, int32_t targetDate);

    void SendReply(const std::string& username, const std::string& text);

    int32_t GetCurrentDate() const;

    // 按用户分片锁，避免全局串行化
    static constexpr size_t LOCK_SHARD_COUNT = 16;
    std::array<std::mutex, LOCK_SHARD_COUNT> cardDataLocks_;
    std::mutex& GetUserLock(const std::string& uid) {
        size_t hash = std::hash<std::string>{}(uid);
        return cardDataLocks_[hash % LOCK_SHARD_COUNT];
    }

    bool inited_ = false;
    std::vector<std::wstring> retroactiveWords_;
    std::vector<std::wstring> queryWords_;

    // 点赞事件监听器 token（用于精确注销）
    size_t likeListenerToken_ = 0;

    RetroactiveCheckInModule();
    ~RetroactiveCheckInModule();
};
