#include "framework.h"
#include "RetroactiveCheckInModule.h"
#include "ProfileManager.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "TextToSpeech.h"
#include "DataBridgeExports.h"
#include "DateUtils.h"
#include "WriteQueue.h"
#include <ctime>
#include <sstream>
#include <algorithm>

#ifdef RUN_UNIT_TESTS
#include <iostream>
#endif

DEFINE_SINGLETON(RetroactiveCheckInModule)

namespace {
    constexpr int32_t STREAK_DAYS_REQUIRED = 7;
    constexpr int32_t MONTHLY_FIRST_LIKES_REQUIRED = 1000;
}

bool RetroactiveCheckInModule::Init() {
    if (inited_) return true;

    LOG_INFO(TEXT("RetroactiveCheckInModule::Init"));

    SetTriggerWords("补签,补签卡;补签查询,补签卡查询,查询补签,查询补签卡,我的补签卡");

    // 注册为点赞事件监听器，保存 token 用于精确注销
    likeListenerToken_ = DanmuProcessor::Inst()->AddLikeEventListener(
        [](const LikeEvent& e) {
            RetroactiveCheckInModule::Inst()->PushLikeEvent(e);
        }
    );

    inited_ = true;
    LOG_INFO(TEXT("RetroactiveCheckInModule::Init done"));
    return true;
}

RetroactiveCheckInModule::RetroactiveCheckInModule() : inited_(false) {
}

RetroactiveCheckInModule::~RetroactiveCheckInModule() {
    // 精确注销本模块注册的点赞事件监听器，不影响其他模块
    if (!DanmuProcessor::GetDestroyingFlag().load() && likeListenerToken_ != 0) {
        DanmuProcessor::Inst()->RemoveLikeEventListener(likeListenerToken_);
    }
    retroactiveWords_.clear();
    queryWords_.clear();
}

void RetroactiveCheckInModule::SetTriggerWords(const std::string& words) {
    retroactiveWords_.clear();
    queryWords_.clear();

    auto parseCsv = [](const std::wstring& input, std::vector<std::wstring>& output) {
        std::wstringstream ss(input);
        std::wstring word;
        while (std::getline(ss, word, L',')) {
            size_t start = word.find_first_not_of(L" \t\r\n");
            if (start != std::wstring::npos) {
                size_t end = word.find_last_not_of(L" \t\r\n");
                word = word.substr(start, end - start + 1);
            } else {
                word.clear();
            }
            if (!word.empty()) {
                output.push_back(word);
            }
        }
    };

    // 使用 ";" 分隔：前面是补签词列表，后面是查询词列表
    std::wstring wwords = Utf8ToWstring(words);
    size_t sepPos = wwords.find(L';');
    std::wstring retroPart = (sepPos != std::wstring::npos) ? wwords.substr(0, sepPos) : wwords;
    std::wstring queryPart = (sepPos != std::wstring::npos) ? wwords.substr(sepPos + 1) : L"";

    parseCsv(retroPart, retroactiveWords_);
    if (!queryPart.empty()) {
        parseCsv(queryPart, queryWords_);
    } else {
        // 兼容旧格式：不含分号时，含"查询"的词归为查询词
        for (auto it = retroactiveWords_.begin(); it != retroactiveWords_.end(); ) {
            if (it->find(L"查询") != std::wstring::npos) {
                queryWords_.push_back(*it);
                it = retroactiveWords_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool RetroactiveCheckInModule::IsRetroactiveMessage(const std::string& content) const {
    try {
        std::wstring wcontent = Utf8ToWstring(content);
        for (const auto& word : retroactiveWords_) {
            if (wcontent == word) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsRetroactiveMessage error: %hs"), e.what());
    }
    return false;
}

bool RetroactiveCheckInModule::IsQueryMessage(const std::string& content) const {
    try {
        std::wstring wcontent = Utf8ToWstring(content);
        for (const auto& word : queryWords_) {
            if (wcontent == word) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsQueryMessage error: %hs"), e.what());
    }
    return false;
}

int32_t RetroactiveCheckInModule::GetCurrentDate() const {
    return DateUtils::GetCurrentDate();
}

void RetroactiveCheckInModule::SendReply(const std::string& username, const std::string& text, bool enableTTS) {
    std::wstring wtext = Utf8ToWstring(text);
    RECORD_HISTORY(wtext.c_str());

    if (!enableTTS || !ConfigManager::Inst()->GetConfig().enableVoice) {
        std::wstring usernameW = Utf8ToWstring(username);
        if (g_aiReplyCallback) {
            g_aiReplyCallback(usernameW.c_str(), wtext.c_str(), g_aiReplyUserData);
        }
    } else {
        TTSManager::Inst()->SpeakCheckinTTS(wtext, username, [username, wtext](bool success, const std::string& errorMsg) {
            if (!success) {
                LOG_ERROR(TEXT("RetroactiveCheckInModule: TTS failed for [%hs], fallback to bubble: %hs"),
                    username.c_str(), errorMsg.c_str());
                std::wstring usernameW = Utf8ToWstring(username);
                if (g_aiReplyCallback) {
                    g_aiReplyCallback(usernameW.c_str(), wtext.c_str(), g_aiReplyUserData);
                }
            }
        });
    }
}

void RetroactiveCheckInModule::PushLikeEvent(const LikeEvent& event) {
    if (!inited_) return;
    
    if (event.likeCount <= 0) return;
    if (event.uid.empty()) return;
    
    std::lock_guard<std::mutex> lock(GetUserLock(event.uid));
    
    // 去重由 DanmuProcessor::IsDuplicateMsgId 和数据库 upsert 保证
    ProcessLike(event);
}

void RetroactiveCheckInModule::ProcessLike(const LikeEvent& event) {
    int32_t date = event.date > 0 ? event.date : GetCurrentDate();
    if (date == 0) return;

    auto* self = this;
    WriteQueue::Inst()->Enqueue(WriteTask{
        .type = WriteTaskType::LIKE,
        .uid = event.uid,
        .username = event.username,
        .execute = [self, event, date](ProfileManager* pm) -> bool {
            int32_t totalLikes = 0;
            if (!pm->AddDailyLike(event.uid, date, event.likeCount, totalLikes)) {
                return false;
            }

            LOG_INFO(TEXT("RetroactiveCheckInModule: Like stored uid=%hs date=%d like_count=%d total_likes=%d"),
                event.uid.c_str(), date, event.likeCount, totalLikes);

            if (totalLikes >= MONTHLY_FIRST_LIKES_REQUIRED) {
                if (self->CheckRule2_MonthlyFirst(event.uid, date, totalLikes)) {
                    std::string reply = event.username + "，恭喜！今日点赞突破1000，获得1张补签卡！";
                    self->SendReply(event.username, reply);
                }
            }

            if (self->CheckRule1_StreakReward(event.uid, date)) {
                std::string reply = event.username + "，恭喜！连续" + std::to_string(STREAK_DAYS_REQUIRED) + "天点赞，获得1张补签卡！";
                self->SendReply(event.username, reply);
            }
            return true;
        }
    });
}

bool RetroactiveCheckInModule::CheckRule1_StreakReward(const std::string& uid, int32_t date) {
    LikeStreakData streak;
    if (!ProfileManager::Inst()->LoadLikeStreak(uid, streak)) {
        return false;
    }

    if (streak.lastLikeDate > 0) {
        int32_t expectedNextDate = 0;
        int32_t lastYear = streak.lastLikeDate / 10000;
        int32_t lastMonth = (streak.lastLikeDate % 10000) / 100;
        int32_t lastDay = streak.lastLikeDate % 100;
        int32_t year = date / 10000;
        int32_t month = (date % 10000) / 100;
        int32_t day = date % 100;

        if (year == lastYear && month == lastMonth) {
            expectedNextDate = streak.lastLikeDate + 1;
        } else if (year == lastYear && month == lastMonth + 1 && day == 1) {
            int32_t lastMonthDays = DateUtils::GetDaysInMonth(lastYear, lastMonth);
            if (lastDay == lastMonthDays) {
                expectedNextDate = date;
            }
        } else if (year == lastYear + 1 && month == 1 && lastMonth == 12 && day == 1 && lastDay == 31) {
            expectedNextDate = date;
        }

        if (date == expectedNextDate) {
            streak.currentStreak++;
        } else if (date != streak.lastLikeDate) {
            streak.currentStreak = 1;
        }
    } else {
        streak.currentStreak = 1;
    }

    streak.lastLikeDate = date;

    // 先保存 streak 状态（不含奖励标记）
    ProfileManager::Inst()->SaveLikeStreak(streak);

    // 检查是否满足奖励条件，使用原子接口发放奖励
    if (streak.currentStreak >= STREAK_DAYS_REQUIRED && 
        (streak.currentStreak % STREAK_DAYS_REQUIRED) == 0 && 
        streak.streakRewardIssued != date) {
        if (ProfileManager::Inst()->IssueStreakReward(uid, date)) {
            LOG_INFO(TEXT("RetroactiveCheckInModule: Rule1 reward issued to uid=%hs"), uid.c_str());
            return true;
        }
    }

    return false;
}

bool RetroactiveCheckInModule::CheckRule2_MonthlyFirst(const std::string& uid, int32_t date, int32_t totalLikes) {
    if (totalLikes < MONTHLY_FIRST_LIKES_REQUIRED) return false;

    RetroactiveCardData cards;
    if (!ProfileManager::Inst()->LoadRetroactiveCards(uid, cards)) {
        return false;
    }

    int32_t currentMonth = date / 100;
    int32_t claimedMonth = cards.monthlyFirstClaimed / 100;

    if (currentMonth != claimedMonth) {
        // 跨月时重置 monthlyFirstClaimed
        cards.monthlyFirstClaimed = 0;
        ProfileManager::Inst()->SaveRetroactiveCards(cards);
    }

    if (cards.monthlyFirstClaimed > 0) {
        return false;
    }

    // 使用原子接口发放奖励
    if (ProfileManager::Inst()->IssueMonthlyFirstReward(uid, date)) {
        LOG_INFO(TEXT("RetroactiveCheckInModule: Rule2 reward issued to uid=%hs"), uid.c_str());
        return true;
    }

    return false;
}

void RetroactiveCheckInModule::PushDanmuEvent(const DanmuProcessor::CaptainDanmuEvent& event) {
    if (!inited_) return;
    if (event.uid.empty()) return;

    // 与点赞路径共享按用户分片锁，保护 retroactive_cards 的读写一致性
    // 补签命令操作（扣卡、插记录、更新profile）与点赞奖励发放（CheckRule1/2）可能并发修改同一用户的卡片数据
    std::lock_guard<std::mutex> lock(GetUserLock(event.uid));

    if (IsQueryMessage(event.content)) {
        HandleQueryCommand(event);
    } else if (IsRetroactiveMessage(event.content)) {
        HandleRetroactiveCommand(event);
    }
}

void RetroactiveCheckInModule::HandleRetroactiveCommand(const DanmuProcessor::CaptainDanmuEvent& event) {
    RetroactiveCardData cards;
    if (!ProfileManager::Inst()->LoadRetroactiveCards(event.uid, cards)) {
        SendReply(event.username, event.username + "，系统错误，请稍后再试。");
        return;
    }

    if (cards.cardCount <= 0) {
        SendReply(event.username, event.username + "，你没有补签卡哦~");
        return;
    }

    int32_t currentDate = event.sendDate > 0 ? event.sendDate : GetCurrentDate();
    if (currentDate == 0) {
        SendReply(event.username, event.username + "，日期获取失败，请稍后再试。");
        return;
    }

    int32_t missingDate = ProfileManager::Inst()->FindLastMissingCheckinDate(event.uid, currentDate);
    if (missingDate == 0) {
        SendReply(event.username, event.username + "，当前没有需要补签的日期。");
        return;
    }

    if (ExecuteRetroactive(event.uid, event.username, missingDate)) {
        int32_t newContinuousDays = ProfileManager::Inst()->CalculateContinuousDays(
            event.uid, missingDate);

        // 重新加载卡片数量，确保显示最新值
        RetroactiveCardData updatedCards;
        int32_t remainingCards = cards.cardCount - 1;
        if (ProfileManager::Inst()->LoadRetroactiveCards(event.uid, updatedCards)) {
            remainingCards = updatedCards.cardCount;
        }

        std::ostringstream oss;
        oss << event.username << "，已成功补签" << (missingDate / 100 % 100) << "月" << (missingDate % 100) << "日，"
            << "剩余补签卡" << remainingCards << "张，连续打卡恢复为" << newContinuousDays << "天！";
        SendReply(event.username, oss.str());
    } else {
        SendReply(event.username, event.username + "，补签失败，请稍后再试。");
    }
}

void RetroactiveCheckInModule::HandleQueryCommand(const DanmuProcessor::CaptainDanmuEvent& event) {
    RetroactiveCardData cards;
    LikeStreakData streak;
    DailyLikeData dailyLike;

    int32_t currentDate = event.sendDate > 0 ? event.sendDate : GetCurrentDate();
    int32_t currentMonth = currentDate / 100;

    bool hasCards = ProfileManager::Inst()->LoadRetroactiveCards(event.uid, cards);
    bool hasStreak = ProfileManager::Inst()->LoadLikeStreak(event.uid, streak);
    bool hasDailyLike = ProfileManager::Inst()->GetDailyLike(event.uid, currentDate, dailyLike);

    if (!hasCards && !hasStreak && !hasDailyLike) {
        SendReply(event.username, event.username + "，系统错误，请稍后再试。", false);
        return;
    }

    std::ostringstream oss;
    oss << event.username << "，补签卡" << cards.cardCount << "张";

    int32_t remainingStreak = STREAK_DAYS_REQUIRED - streak.currentStreak;
    if (remainingStreak <= 0) {
        oss << "\n连续点赞" << STREAK_DAYS_REQUIRED << "天：已满足，下次领取";
    } else {
        oss << "\n连续点赞" << STREAK_DAYS_REQUIRED << "天：已" << streak.currentStreak << "天，差" << remainingStreak << "天";
    }

    int32_t claimedMonth = cards.monthlyFirstClaimed / 100;
    if (claimedMonth == currentMonth && cards.monthlyFirstClaimed > 0) {
        oss << "\n月度点赞1000：已领取";
    } else {
        int32_t currentLikes = dailyLike.totalLikes;
        int32_t remainingLikes = MONTHLY_FIRST_LIKES_REQUIRED - currentLikes;
        if (remainingLikes <= 0) {
            oss << "\n月度点赞1000：已满足，可领取";
        } else {
            oss << "\n月度点赞1000：" << currentLikes << "/" << MONTHLY_FIRST_LIKES_REQUIRED << "，差" << remainingLikes;
        }
    }

    SendReply(event.username, oss.str(), false);
}

bool RetroactiveCheckInModule::ExecuteRetroactive(const std::string& uid, const std::string& username, int32_t targetDate) {
    int32_t newCardCount = 0;
    if (!ProfileManager::Inst()->ExecuteRetroactiveCheckin(uid, username, targetDate, newCardCount)) {
        LOG_ERROR(TEXT("RetroactiveCheckInModule: Atomic retroactive checkin failed for uid=%hs, date=%d"), uid.c_str(), targetDate);
        return false;
    }

    LOG_INFO(TEXT("RetroactiveCheckInModule: Retroactive checkin executed for uid=%hs, date=%d, remaining cards=%d"), uid.c_str(), targetDate, newCardCount);
    return true;
}
