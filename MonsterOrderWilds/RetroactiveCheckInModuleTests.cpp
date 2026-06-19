#include "framework.h"
#include "RetroactiveCheckInModule.h"
#include "ProfileManager.h"
#include <sqlite3.h>
#include <cassert>
#include <iostream>
#include <atomic>
#include <thread>

#ifdef RUN_UNIT_TESTS

static void TestLog(const char* msg) {
    std::cout << msg << std::endl;
}

static void CleanupTestData() {
    sqlite3* db = (sqlite3*)ProfileManager::Inst()->GetStorage();
    if (db) {
        sqlite3_exec(db, "DELETE FROM user_profiles WHERE uid LIKE 'test_%' OR uid = 'streak_user' OR uid = 'monthly_user' OR uid = 'cross_month_user' OR uid = '12345' OR uid = '67890' OR uid = '99999' OR uid LIKE 'concurrent_%'", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "DELETE FROM user_daily_likes WHERE uid LIKE 'test_%' OR uid = 'streak_user' OR uid = 'monthly_user' OR uid = 'cross_month_user'", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "DELETE FROM user_like_streaks WHERE uid LIKE 'test_%' OR uid = 'streak_user' OR uid = 'monthly_user' OR uid = 'cross_month_user'", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "DELETE FROM retroactive_cards WHERE uid LIKE 'test_%' OR uid = 'streak_user' OR uid = 'monthly_user' OR uid = 'cross_month_user' OR uid = '12345' OR uid = '67890' OR uid = '99999' OR uid LIKE 'concurrent_%'", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "DELETE FROM checkin_records WHERE uid LIKE 'test_%' OR uid = 'streak_user' OR uid = 'monthly_user' OR uid = 'cross_month_user' OR uid = '12345' OR uid = '67890' OR uid = '99999' OR uid LIKE 'concurrent_%'", nullptr, nullptr, nullptr);
    }
}

void TestLikeEventParsing() {
    CleanupTestData();
    json j = {
        {"cmd", "LIVE_OPEN_PLATFORM_LIKE"},
        {"data", {
            {"open_id", "test_uid_123"},
            {"uname", "TestUser"},
            {"like_count", 15},
            {"timestamp", 1777030207}
        }}
    };

    LikeEvent event = DanmuProcessor::Inst()->ParseLikeJson(j);
    assert(event.uid == "test_uid_123");
    assert(event.username == "TestUser");
    assert(event.likeCount == 15);
    assert(event.timestamp == 1777030207);
    assert(event.date == 20260424);

    TestLog("[PASS] TestLikeEventParsing");
}

void TestDailyLikeTracking() {
    CleanupTestData();
    ProfileManager::Inst()->Init();

    int32_t totalLikes = 0;
    assert(ProfileManager::Inst()->AddDailyLike("user1", 20260424, 5, totalLikes));
    assert(totalLikes == 5);

    assert(ProfileManager::Inst()->AddDailyLike("user1", 20260424, 10, totalLikes));
    assert(totalLikes == 15);

    assert(ProfileManager::Inst()->AddDailyLike("user1", 20260425, 8, totalLikes));
    assert(totalLikes == 8);

    DailyLikeData data;
    assert(ProfileManager::Inst()->GetDailyLike("user1", 20260424, data));
    assert(data.totalLikes == 15);

    TestLog("[PASS] TestDailyLikeTracking");
}

void TestStreakReward() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    for (int i = 0; i < 7; ++i) {
        LikeEvent evt;
        evt.uid = "streak_user";
        evt.username = "StreakUser";
        evt.likeCount = 1;
        evt.date = 20260418 + i;
        evt.timestamp = 1777000000 + i * 86400;

        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }

    RetroactiveCardData cards;
    assert(ProfileManager::Inst()->LoadRetroactiveCards("streak_user", cards));
    assert(cards.cardCount == 1);
    assert(cards.totalEarned == 1);

    TestLog("[PASS] TestStreakReward");
}

void TestStreakBoundary() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 连续6天不应发放
    for (int i = 0; i < 6; ++i) {
        LikeEvent evt;
        evt.uid = "test_boundary_streak";
        evt.username = "BoundaryUser";
        evt.likeCount = 1;
        evt.date = 20260418 + i;
        evt.timestamp = 1777000000 + i * 86400;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }

    RetroactiveCardData cards;
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_streak", cards));
    assert(cards.cardCount == 0); // 6天不应奖励

    // 第7天应发放
    LikeEvent evt7;
    evt7.uid = "test_boundary_streak";
    evt7.username = "BoundaryUser";
    evt7.likeCount = 1;
    evt7.date = 20260424;
    evt7.timestamp = 1777000000 + 6 * 86400;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt7);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_streak", cards));
    assert(cards.cardCount == 1); // 7天应奖励

    // 第8天不应重复发放
    LikeEvent evt8;
    evt8.uid = "test_boundary_streak";
    evt8.username = "BoundaryUser";
    evt8.likeCount = 1;
    evt8.date = 20260425;
    evt8.timestamp = 1777000000 + 7 * 86400;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt8);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_streak", cards));
    assert(cards.cardCount == 1); // 8天不应重复奖励

    // 第14天应再次发放
    for (int i = 8; i < 14; ++i) {
        LikeEvent evt;
        evt.uid = "test_boundary_streak";
        evt.username = "BoundaryUser";
        evt.likeCount = 1;
        evt.date = 20260418 + i;
        evt.timestamp = 1777000000 + i * 86400;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }

    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_streak", cards));
    assert(cards.cardCount == 2); // 14天应再次奖励

    TestLog("[PASS] TestStreakBoundary");
}

void TestMonthlyFirstReward() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    LikeEvent evt;
    evt.uid = "monthly_user";
    evt.username = "MonthlyUser";
    evt.likeCount = 35;
    evt.date = 20260424;
    evt.timestamp = 1777030207;

    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    RetroactiveCardData cards;
    assert(ProfileManager::Inst()->LoadRetroactiveCards("monthly_user", cards));
    assert(cards.cardCount == 1);
    assert(cards.monthlyFirstClaimed == 20260424);

    evt.likeCount = 10;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("monthly_user", cards));
    assert(cards.cardCount == 1);

    TestLog("[PASS] TestMonthlyFirstReward");
}

void TestMonthlyFirstBoundary() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 29赞不应发放
    LikeEvent evt;
    evt.uid = "test_boundary_monthly";
    evt.username = "BoundaryUser";
    evt.likeCount = 29;
    evt.date = 20260424;
    evt.timestamp = 1777030207;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    RetroactiveCardData cards;
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_monthly", cards));
    assert(cards.cardCount == 0); // 29不应奖励

    // 再点1赞达到30，应发放
    evt.likeCount = 1;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_monthly", cards));
    assert(cards.cardCount == 1); // 30应奖励

    // 同一天再次超过30，不应重复发放
    evt.likeCount = 10;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_boundary_monthly", cards));
    assert(cards.cardCount == 1); // 不应重复奖励

    TestLog("[PASS] TestMonthlyFirstBoundary");
}

void TestCrossMonthReset() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    LikeStreakData streak;
    streak.uid = "cross_month_user";
    streak.currentStreak = 3;
    streak.lastLikeDate = 20260428;
    streak.streakRewardIssued = 0;
    ProfileManager::Inst()->SaveLikeStreak(streak);

    RetroactiveCardData cards;
    cards.uid = "cross_month_user";
    cards.cardCount = 0;
    cards.totalEarned = 0;
    cards.monthlyFirstClaimed = 20260424;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    LikeEvent evt;
    evt.uid = "cross_month_user";
    evt.username = "CrossMonthUser";
    evt.likeCount = 35;
    evt.date = 20260501;
    evt.timestamp = 1777600000;

    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("cross_month_user", cards));
    assert(cards.cardCount == 1);
    assert(cards.monthlyFirstClaimed == 20260501);

    TestLog("[PASS] TestCrossMonthReset");
}

void TestLeapYearAndCrossYear() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 闰年 2024-02-28 -> 2024-02-29 -> 2024-03-01
    {
        LikeEvent evt;
        evt.uid = "test_leap_year";
        evt.username = "LeapUser";
        evt.likeCount = 1;
        evt.date = 20240228;
        evt.timestamp = 1709078400;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }
    {
        LikeEvent evt;
        evt.uid = "test_leap_year";
        evt.username = "LeapUser";
        evt.likeCount = 1;
        evt.date = 20240229;
        evt.timestamp = 1709164800;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }
    {
        LikeEvent evt;
        evt.uid = "test_leap_year";
        evt.username = "LeapUser";
        evt.likeCount = 1;
        evt.date = 20240301;
        evt.timestamp = 1709251200;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }

    LikeStreakData streak;
    assert(ProfileManager::Inst()->LoadLikeStreak("test_leap_year", streak));
    assert(streak.currentStreak == 3);

    // 跨年 2024-12-31 -> 2025-01-01
    CleanupTestData();
    {
        LikeEvent evt;
        evt.uid = "test_cross_year";
        evt.username = "CrossYearUser";
        evt.likeCount = 1;
        evt.date = 20241231;
        evt.timestamp = 1735689600;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }
    {
        LikeEvent evt;
        evt.uid = "test_cross_year";
        evt.username = "CrossYearUser";
        evt.likeCount = 1;
        evt.date = 20250101;
        evt.timestamp = 1735776000;
        RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    }

    assert(ProfileManager::Inst()->LoadLikeStreak("test_cross_year", streak));
    assert(streak.currentStreak == 2);

    TestLog("[PASS] TestLeapYearAndCrossYear");
}

void TestRetroactiveCommand() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "12345";
    cards.cardCount = 2;
    cards.totalEarned = 2;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    ProfileManager::Inst()->InsertRetroactiveCheckin("12345", "RetroUser", 20260420);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "12345";
    evt.username = "RetroUser";
    evt.content = "补签";
    evt.sendDate = 20260424;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    assert(ProfileManager::Inst()->LoadRetroactiveCards("12345", cards));
    assert(cards.cardCount == 1);

    TestLog("[PASS] TestRetroactiveCommand");
}

void TestRetroactiveCommandWithHistoricalGap() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 创建补签卡
    RetroactiveCardData cards;
    cards.uid = "test_gap_user";
    cards.cardCount = 3;
    cards.totalEarned = 3;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    // 创建打卡记录：20260601, 20260603, 20260604, 20260605 (缺失20260602)
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_gap_user", "GapUser", 20260601);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_gap_user", "GapUser", 20260603);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_gap_user", "GapUser", 20260604);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_gap_user", "GapUser", 20260605);

    // 用户今天已打卡（20260605），但20260602缺失
    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_gap_user";
    evt.username = "GapUser";
    evt.content = "补签";
    evt.sendDate = 20260605;  // 今天已打卡

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证补签成功（应该补签20260602）
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_gap_user", cards));
    assert(cards.cardCount == 2);  // 3-1=2

    // 验证连续打卡天数正确更新（20260601→20260602→20260603→20260604→20260605 = 5天）
    UserProfileData profile;
    assert(ProfileManager::Inst()->LoadProfile("test_gap_user", profile));
    assert(profile.continuousDays == 5);

    TestLog("[PASS] TestRetroactiveCommandWithHistoricalGap");
}

void TestRetroactiveCommandNoGap() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 创建补签卡
    RetroactiveCardData cards;
    cards.uid = "test_no_gap";
    cards.cardCount = 2;
    cards.totalEarned = 2;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    // 创建连续打卡记录：20260603, 20260604, 20260605
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_gap", "NoGapUser", 20260603);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_gap", "NoGapUser", 20260604);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_gap", "NoGapUser", 20260605);

    // 用户今天已打卡（20260605），连续打卡无缺失
    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_no_gap";
    evt.username = "NoGapUser";
    evt.content = "补签";
    evt.sendDate = 20260605;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证没有补签（卡片数量不变）
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_no_gap", cards));
    assert(cards.cardCount == 2);  // 未使用

    TestLog("[PASS] TestRetroactiveCommandNoGap");
}

void TestQueryCommand() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "67890";
    cards.cardCount = 3;
    cards.totalEarned = 3;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    LikeStreakData streak;
    streak.uid = "67890";
    streak.currentStreak = 5;
    streak.lastLikeDate = 20260424;
    streak.streakRewardIssued = 0;
    ProfileManager::Inst()->SaveLikeStreak(streak);

    ProfileManager::Inst()->AddDailyLike("67890", 20260424, 15, streak.currentStreak);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "67890";
    evt.username = "QueryUser";
    evt.content = "补签查询";
    evt.sendDate = 20260424;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    TestLog("[PASS] TestQueryCommand");
}

void TestNoCardRetroactive() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "test_no_card";
    cards.cardCount = 0;
    cards.totalEarned = 0;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_card", "NoCardUser", 20260420);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_no_card";
    evt.username = "NoCardUser";
    evt.content = "补签";
    evt.sendDate = 20260424;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证卡片数量仍为0
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_no_card", cards));
    assert(cards.cardCount == 0);

    TestLog("[PASS] TestNoCardRetroactive");
}

void TestNoMissingDateRetroactive() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "test_no_missing";
    cards.cardCount = 2;
    cards.totalEarned = 2;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    // 连续打卡到昨天，没有缺失
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_missing", "NoMissingUser", 20260420);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_missing", "NoMissingUser", 20260421);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_missing", "NoMissingUser", 20260422);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_no_missing", "NoMissingUser", 20260423);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_no_missing";
    evt.username = "NoMissingUser";
    evt.content = "补签";
    evt.sendDate = 20260424;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证卡片数量未减少（没有补签）
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_no_missing", cards));
    assert(cards.cardCount == 2);

    TestLog("[PASS] TestNoMissingDateRetroactive");
}

void TestEmptyUid() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    LikeEvent evt;
    evt.uid = "";
    evt.username = "EmptyUser";
    evt.likeCount = 1;
    evt.date = 20260424;
    evt.timestamp = 1777030207;

    // 不应崩溃，不应影响数据库
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    TestLog("[PASS] TestEmptyUid");
}

void TestInvalidLikeCount() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    LikeEvent evt;
    evt.uid = "test_invalid_like";
    evt.username = "InvalidUser";
    evt.likeCount = 0;
    evt.date = 20260424;
    evt.timestamp = 1777030207;

    // likeCount <= 0 应被忽略
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);

    DailyLikeData data;
    bool hasData = ProfileManager::Inst()->GetDailyLike("test_invalid_like", 20260424, data);
    assert(!hasData || data.totalLikes == 0);

    TestLog("[PASS] TestInvalidLikeCount");
}

void TestConcurrentDeduction() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    // 准备：1个用户，持有5张卡，有缺失日期
    std::string uid = "concurrent_single_user";
    RetroactiveCardData cards;
    cards.uid = uid;
    cards.cardCount = 5;
    cards.totalEarned = 5;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    ProfileManager::Inst()->InsertRetroactiveCheckin(uid, "ConcurrentUser", 20260420);

    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([uid, &successCount]() {
            DanmuProcessor::CaptainDanmuEvent evt;
            evt.uid = uid;
            evt.username = "ConcurrentUser";
            evt.content = "补签";
            evt.sendDate = 20260424;

            RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);
            successCount++;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(successCount == 5);
    assert(ProfileManager::Inst()->LoadRetroactiveCards(uid, cards));
    // FindLastMissingCheckinDate 只返回一个缺失日期(20260423)，第一次线程成功扣卡并插入记录后
    // 后续线程发现无缺失日期就返回了，所以只扣了1张卡
    assert(cards.cardCount == 4); // 初始5张，补签成功1次，剩余4张

    TestLog("[PASS] TestConcurrentDeduction");
}

// 补签有效性校验：连续打卡天数 >= 累计打卡天数时补签无效（满勤场景）
// 构造 20260601/02/03 连续 3 天打卡，sendDate=20260603（今天已打卡）
// continuousDays==3, cumulativeDays==3，应触发早判断，不扣卡、不插记录
void TestRetroactiveInvalid_FullAttendance() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "test_full_attendance";
    cards.cardCount = 2;
    cards.totalEarned = 2;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    // 连续 3 天打卡（满勤，无缺失）
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_full_attendance", "FullUser", 20260601);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_full_attendance", "FullUser", 20260602);
    ProfileManager::Inst()->InsertRetroactiveCheckin("test_full_attendance", "FullUser", 20260603);

    // 验证前置条件：continuous==cumulative==3
    int32_t continuous = ProfileManager::Inst()->CalculateContinuousDaysFromRecords("test_full_attendance");
    int32_t cumulative = ProfileManager::Inst()->GetCumulativeDaysFromRecords("test_full_attendance");
    assert(continuous == 3);
    assert(cumulative == 3);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_full_attendance";
    evt.username = "FullUser";
    evt.content = "补签";
    evt.sendDate = 20260603;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证补签无效：卡数不变、累计天数不变
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_full_attendance", cards));
    assert(cards.cardCount == 2);
    assert(ProfileManager::Inst()->GetCumulativeDaysFromRecords("test_full_attendance") == 3);

    TestLog("[PASS] TestRetroactiveInvalid_FullAttendance");
}

// 补签有效性校验：单天打卡场景（continuous==1 >= cumulative==1）
// 即使用户只有 1 天记录且当天就是 sendDate，也应被拒绝补签
void TestRetroactiveInvalid_SingleDay() {
    CleanupTestData();
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "test_single_day";
    cards.cardCount = 3;
    cards.totalEarned = 3;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    ProfileManager::Inst()->InsertRetroactiveCheckin("test_single_day", "SingleUser", 20260605);

    int32_t continuous = ProfileManager::Inst()->CalculateContinuousDaysFromRecords("test_single_day");
    int32_t cumulative = ProfileManager::Inst()->GetCumulativeDaysFromRecords("test_single_day");
    assert(continuous == 1);
    assert(cumulative == 1);

    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "test_single_day";
    evt.username = "SingleUser";
    evt.content = "补签";
    evt.sendDate = 20260605;

    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);

    // 验证补签无效：卡数不变
    assert(ProfileManager::Inst()->LoadRetroactiveCards("test_single_day", cards));
    assert(cards.cardCount == 3);

    TestLog("[PASS] TestRetroactiveInvalid_SingleDay");
}

void RunRetroactiveCheckInModuleTests() {
    std::cout << "========== RetroactiveCheckInModule Tests ==========" << std::endl;
    TestLikeEventParsing();
    TestDailyLikeTracking();
    TestStreakReward();
    TestStreakBoundary();
    TestMonthlyFirstReward();
    TestMonthlyFirstBoundary();
    TestCrossMonthReset();
    TestLeapYearAndCrossYear();
    TestRetroactiveCommand();
    TestQueryCommand();
    TestNoCardRetroactive();
    TestNoMissingDateRetroactive();
    TestEmptyUid();
    TestInvalidLikeCount();
    TestRetroactiveCommandWithHistoricalGap();
    TestRetroactiveCommandNoGap();
    TestConcurrentDeduction();
    TestRetroactiveInvalid_FullAttendance();
    TestRetroactiveInvalid_SingleDay();
    std::cout << "========== All RetroactiveCheckInModule Tests Passed ==========" << std::endl;
}

#endif // RUN_UNIT_TESTS
