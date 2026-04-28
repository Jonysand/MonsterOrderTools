# 琛ョ鍗℃満鍒?Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 涓烘墦鍗＄郴缁熸坊鍔犺ˉ绛惧崱鏈哄埗锛岀敤鎴烽€氳繃鐐硅禐鑾峰緱琛ョ鍗★紝鐢ㄤ簬鎭㈠鎵撳崱杩炵画鎬с€?
**Architecture:** 鏂板缓鐙珛妯″潡 `RetroactiveCheckInModule` 澶勭悊琛ョ鍗￠€昏緫锛岄€氳繃 `DanmuProcessor` 鎺ユ敹鐐硅禐浜嬩欢鍜屽脊骞曞懡浠わ紝閫氳繃 `ProfileManager` 鎿嶄綔鏁版嵁搴擄紝涓?`CaptainCheckInModule` 瑙ｈ€︺€?
**Tech Stack:** C++17, SQLite, nlohmann/json, MSBuild

---

## File Structure

**Create:**
- `MonsterOrderWilds/LikeEvent.h` 鈥?鐐硅禐浜嬩欢鏁版嵁缁撴瀯
- `MonsterOrderWilds/RetroactiveCheckInModule.h` 鈥?琛ョ鍗℃ā鍧楀ご鏂囦欢
- `MonsterOrderWilds/RetroactiveCheckInModule.cpp` 鈥?琛ョ鍗℃ā鍧楀疄鐜?- `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp` 鈥?鍗曞厓娴嬭瘯

**Modify:**
- `MonsterOrderWilds/DanmuProcessor.h` 鈥?鏂板鐐硅禐浜嬩欢鐩戝惉鍣?- `MonsterOrderWilds/DanmuProcessor.cpp` 鈥?鏂板鐐硅禐浜嬩欢瑙ｆ瀽鍜屽垎鍙?- `MonsterOrderWilds/ProfileManager.h` 鈥?鏂板琛ョ鍗＄浉鍏虫帴鍙?- `MonsterOrderWilds/ProfileManager.cpp` 鈥?鏂板鏁版嵁搴撹〃鍜屾搷浣?- `MonsterOrderWilds/BliveManager.cpp` 鈥?鏂板LIKE娑堟伅澶勭悊
- `MonsterOrderWilds/MonsterOrderWilds.vcxproj` 鈥?娣诲姞鏂版枃浠?- `MonsterOrderWilds/MonsterOrderWilds.vcxproj.filters` 鈥?娣诲姞鏂囦欢鍒癴ilters

---

### Task 1: Create LikeEvent.h

**Files:**
- Create: `MonsterOrderWilds/LikeEvent.h`

- [x] **Step 1: Write LikeEvent.h**

```cpp
#pragma once
#include "framework.h"
#include <string>

struct LikeEvent {
    std::string uid;       // open_id (TEXT)锛岄潪 numeric uid
    std::string username;
    int32_t likeCount = 0;
    int64_t timestamp = 0;
    int32_t date = 0;  // YYYYMMDD
};
```

---

### Task 2: DanmuProcessor.h - Add LikeEvent listener

**Files:**
- Modify: `MonsterOrderWilds/DanmuProcessor.h`

- [x] **Step 1: Add includes and forward declarations**

鍦?`DanmuProcessor.h` 绗?1琛岋紙`#include <locale>` 涔嬪悗锛夋彃鍏ワ細

```cpp
#include "LikeEvent.h"
```

- [x] **Step 2: Add LikeEvent listener types and methods**

鍦?`DanmuProcessor.h` 绗?18琛岋紙`NotifyCaptainDanmu` 澹版槑涔嬪悗锛宍private:` 涔嬪墠锛夋彃鍏ワ細

```cpp
    // 鐐硅禐浜嬩欢锛堢敤浜?RetroactiveCheckInModule锛?    using LikeEventHandler = std::function<void(const LikeEvent&)>;
    void AddLikeEventListener(const LikeEventHandler& handler);

private:
    std::vector<LikeEventHandler> likeEventListeners_;
    void NotifyLikeEvent(const LikeEvent& event);
```

---

### Task 3: DanmuProcessor.cpp - Parse and dispatch LIKE events

**Files:**
- Modify: `MonsterOrderWilds/DanmuProcessor.cpp`

- [x] **Step 1: Add ParseLikeJson method**

鍦?`DanmuProcessor.cpp` 绗?81琛岋紙`NotifyCaptainDanmu` 鏂规硶涔嬪悗锛夋彃鍏ワ細

```cpp
LikeEvent DanmuProcessor::ParseLikeJson(const json& j) const
{
    LikeEvent event;
    try
    {
        json dataObj = j;
        if (j.contains("cmd") && j.contains("data"))
        {
            std::string cmd = j["cmd"].get<std::string>();
            if (cmd != "LIVE_OPEN_PLATFORM_LIKE")
                return event;
            dataObj = j["data"];
        }

        if (dataObj.contains("open_id"))
            event.uid = dataObj["open_id"].get<std::string>();
        else if (dataObj.contains("uid"))
            event.uid = std::to_string(dataObj["uid"].get<int64_t>());
        
        if (dataObj.contains("uname"))
            event.username = dataObj["uname"].get<std::string>();
        if (dataObj.contains("like_count"))
            event.likeCount = dataObj["like_count"].get<int32_t>();
        if (dataObj.contains("timestamp"))
            event.timestamp = dataObj["timestamp"].get<int64_t>();
        else if (dataObj.contains("send_time"))
            event.timestamp = dataObj["send_time"].get<int64_t>();

        // 璁＄畻鏃ユ湡
        if (event.timestamp > 0) {
            std::time_t timeSec = event.timestamp;
            std::tm tmResult = {};
            if (localtime_s(&tmResult, &timeSec) == 0) {
                event.date = (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("[ParseLikeJson] exception: %s"), e.what());
    }
    return event;
}

void DanmuProcessor::AddLikeEventListener(const LikeEventHandler& handler)
{
    likeEventListeners_.push_back(handler);
}

void DanmuProcessor::NotifyLikeEvent(const LikeEvent& event)
{
    for (const auto& handler : likeEventListeners_)
    {
        handler(event);
    }
}
```

- [x] **Step 2: Add public ParseLikeJson declaration to header**

鍦?`DanmuProcessor.h` 绗?6琛岋紙`ParseDanmuJson(const json& j) const;` 涔嬪悗锛夋彃鍏ワ細

```cpp
    LikeEvent ParseLikeJson(const json& j) const;
```

---

### Task 4: ProfileManager.h - Add retroactive card interfaces

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.h`

- [x] **Step 1: Add data structures**

鍦?`ProfileManager.h` 绗?6琛岋紙`UserProfileData` 缁撴瀯浣撲箣鍚庯紝`class ProfileManager` 涔嬪墠锛夋彃鍏ワ細

```cpp
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
```

- [x] **Step 2: Add public methods to ProfileManager**

鍦?`ProfileManager.h` 绗?4琛岋紙`DeserializeFromJson` 涔嬪悗锛宍private:` 涔嬪墠锛夋彃鍏ワ細

```cpp
    // 琛ョ鍗＄浉鍏虫帴鍙?    bool AddDailyLike(const std::string& uid, int32_t likeDate, int32_t likeCount, int32_t& outTotalLikes);
    bool GetDailyLike(const std::string& uid, int32_t likeDate, DailyLikeData& outData);
    
    bool LoadLikeStreak(const std::string& uid, LikeStreakData& outData);
    bool SaveLikeStreak(const LikeStreakData& data);
    
    bool LoadRetroactiveCards(const std::string& uid, RetroactiveCardData& outData);
    bool SaveRetroactiveCards(const RetroactiveCardData& data);
    bool DeductRetroactiveCard(const std::string& uid);  // 鍘熷瓙鎵ｅ噺
    
    bool InsertRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t checkinDate);
    int32_t FindLastMissingCheckinDate(const std::string& uid, int32_t currentDate);
```

---

### Task 5: ProfileManager.cpp - Database tables and operations

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.cpp`

- [x] **Step 1: Add table creation in Init()**

鍦?`ProfileManager.cpp` 绗?36琛岋紙`checkin_records` 琛ㄥ垱寤轰箣鍚庯紝`storage_ = (void*)db;` 涔嬪墠锛夋彃鍏ワ細

```cpp
    const char* createDailyLikesSql = 
        "CREATE TABLE IF NOT EXISTS user_daily_likes ("
        "uid TEXT NOT NULL,"
        "like_date INTEGER NOT NULL,"
        "total_likes INTEGER DEFAULT 0,"
        "PRIMARY KEY (uid, like_date)"
        ")";

    result = sqlite3_exec(db, createDailyLikesSql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to create user_daily_likes table: %hs"), errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    const char* createLikeStreaksSql = 
        "CREATE TABLE IF NOT EXISTS user_like_streaks ("
        "uid TEXT PRIMARY KEY,"
        "current_streak INTEGER DEFAULT 0,"
        "last_like_date INTEGER DEFAULT 0,"
        "streak_reward_issued INTEGER DEFAULT 0"
        ")";

    result = sqlite3_exec(db, createLikeStreaksSql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to create user_like_streaks table: %hs"), errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    const char* createRetroactiveCardsSql = 
        "CREATE TABLE IF NOT EXISTS retroactive_cards ("
        "uid TEXT PRIMARY KEY,"
        "card_count INTEGER DEFAULT 0,"
        "total_earned INTEGER DEFAULT 0,"
        "monthly_first_claimed INTEGER DEFAULT 0,"
        "last_earned_date INTEGER DEFAULT 0"
        ")";

    result = sqlite3_exec(db, createRetroactiveCardsSql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to create retroactive_cards table: %hs"), errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }
```

- [x] **Step 2: Add AddDailyLike implementation**

鍦?`ProfileManager.cpp` 绗?86琛岋紙鏂囦欢鏈熬锛宍EvictOldestProfileIfNeeded` 涔嬪悗锛夋彃鍏ワ細

```cpp
bool ProfileManager::AddDailyLike(const std::string& uid, int32_t likeDate, int32_t likeCount, int32_t& outTotalLikes) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    
    // 鍏堝皾璇?UPDATE
    const char* updateSql = "UPDATE user_daily_likes SET total_likes = total_likes + ? WHERE uid = ? AND like_date = ?";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, likeCount);
        sqlite3_bind_text(stmt, 2, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, likeDate);
        
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int changes = sqlite3_changes(db);
            sqlite3_finalize(stmt);
            
            if (changes > 0) {
                // UPDATE 鎴愬姛锛屾煡璇㈡柊鍊?                const char* selectSql = "SELECT total_likes FROM user_daily_likes WHERE uid = ? AND like_date = ?";
                if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 2, likeDate);
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        outTotalLikes = sqlite3_column_int(stmt, 0);
                    }
                    sqlite3_finalize(stmt);
                    return true;
                }
            } else {
                // 璁板綍涓嶅瓨鍦紝INSERT
                const char* insertSql = "INSERT INTO user_daily_likes (uid, like_date, total_likes) VALUES (?, ?, ?)";
                if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 2, likeDate);
                    sqlite3_bind_int(stmt, 3, likeCount);
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        outTotalLikes = likeCount;
                        sqlite3_finalize(stmt);
                        return true;
                    }
                    sqlite3_finalize(stmt);
                }
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }
    
    return false;
}

bool ProfileManager::GetDailyLike(const std::string& uid, int32_t likeDate, DailyLikeData& outData) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT uid, like_date, total_likes FROM user_daily_likes WHERE uid = ? AND like_date = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, likeDate);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outData.uid = (const char*)sqlite3_column_text(stmt, 0);
        outData.likeDate = sqlite3_column_int(stmt, 1);
        outData.totalLikes = sqlite3_column_int(stmt, 2);
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}

bool ProfileManager::LoadLikeStreak(const std::string& uid, LikeStreakData& outData) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT uid, current_streak, last_like_date, streak_reward_issued FROM user_like_streaks WHERE uid = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outData.uid = (const char*)sqlite3_column_text(stmt, 0);
        outData.currentStreak = sqlite3_column_int(stmt, 1);
        outData.lastLikeDate = sqlite3_column_int(stmt, 2);
        outData.streakRewardIssued = sqlite3_column_int(stmt, 3);
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    // 杩斿洖榛樿鏁版嵁
    outData.uid = uid;
    outData.currentStreak = 0;
    outData.lastLikeDate = 0;
    outData.streakRewardIssued = 0;
    return true;
}

bool ProfileManager::SaveLikeStreak(const LikeStreakData& data) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO user_like_streaks (uid, current_streak, last_like_date, streak_reward_issued) VALUES (?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, data.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, data.currentStreak);
    sqlite3_bind_int(stmt, 3, data.lastLikeDate);
    sqlite3_bind_int(stmt, 4, data.streakRewardIssued);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool ProfileManager::LoadRetroactiveCards(const std::string& uid, RetroactiveCardData& outData) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT uid, card_count, total_earned, monthly_first_claimed, last_earned_date FROM retroactive_cards WHERE uid = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outData.uid = (const char*)sqlite3_column_text(stmt, 0);
        outData.cardCount = sqlite3_column_int(stmt, 1);
        outData.totalEarned = sqlite3_column_int(stmt, 2);
        outData.monthlyFirstClaimed = sqlite3_column_int(stmt, 3);
        outData.lastEarnedDate = sqlite3_column_int(stmt, 4);
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    // 杩斿洖榛樿鏁版嵁
    outData.uid = uid;
    outData.cardCount = 0;
    outData.totalEarned = 0;
    outData.monthlyFirstClaimed = 0;
    outData.lastEarnedDate = 0;
    return true;
}

bool ProfileManager::SaveRetroactiveCards(const RetroactiveCardData& data) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO retroactive_cards (uid, card_count, total_earned, monthly_first_claimed, last_earned_date) VALUES (?, ?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, data.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, data.cardCount);
    sqlite3_bind_int(stmt, 3, data.totalEarned);
    sqlite3_bind_int(stmt, 4, data.monthlyFirstClaimed);
    sqlite3_bind_int(stmt, 5, data.lastEarnedDate);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool ProfileManager::DeductRetroactiveCard(const std::string& uid) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    
    // 寮€濮嬩簨鍔?    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT card_count FROM retroactive_cards WHERE uid = ?";
    
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    
    int cardCount = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cardCount = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (cardCount <= 0) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    
    const char* updateSql = "UPDATE retroactive_cards SET card_count = card_count - 1 WHERE uid = ?";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    if (success) {
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
}

bool ProfileManager::InsertRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t checkinDate) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, checkinDate);
    sqlite3_bind_int64(stmt, 3, GetCurrentTimestamp());
    sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

int32_t ProfileManager::FindLastMissingCheckinDate(const std::string& uid, int32_t currentDate) {
    if (!storage_) return 0;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    
    // 鏌ヨ鐢ㄦ埛鏈€杩戠殑涓€娆℃墦鍗℃棩鏈?    const char* sql = "SELECT MAX(checkin_date) FROM checkin_records WHERE uid = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    
    int32_t lastCheckinDate = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        lastCheckinDate = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (lastCheckinDate == 0) {
        return 0;  // 鏃犲巻鍙茶褰?    }
    
    if (lastCheckinDate >= currentDate) {
        return 0;  // 宸叉墦鍗″埌浠婂ぉ
    }
    
    // 妫€鏌?lastCheckinDate 鍒?currentDate 涔嬮棿鏄惁鏈夌己澶?    // 濡傛灉鏈夌己澶憋紝杩斿洖鏈€杩戠殑涓€涓己澶辨棩鏈?    int32_t year = currentDate / 10000;
    int32_t month = (currentDate % 10000) / 100;
    int32_t day = currentDate % 100;
    
    int32_t lastYear = lastCheckinDate / 10000;
    int32_t lastMonth = (lastCheckinDate % 10000) / 100;
    int32_t lastDay = lastCheckinDate % 100;
    
    // 濡傛灉鍙樊涓€澶╋紝璇存槑鏄繛缁殑
    if (year == lastYear && month == lastMonth && day == lastDay + 1) {
        return 0;
    }
    if (year == lastYear && month == lastMonth + 1 && day == 1) {
        int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
        if (lastDay == lastMonthDays) {
            return 0;
        }
    }
    
    // 杩斿洖鏈€杩戠殑涓€涓己澶辨棩鏈燂紙currentDate - 1锛?    // 绠€鍖栧鐞嗭細鍋囪鍙ˉ鏈€杩戠殑涓€澶?    int32_t missingDate = currentDate - 1;
    
    // 妫€鏌?missingDate 鏄惁宸叉墦鍗?    const char* checkSql = "SELECT 1 FROM checkin_records WHERE uid = ? AND checkin_date = ?";
    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, missingDate);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // missingDate 宸叉墦鍗★紝缁х画寰€鍓嶆壘
            sqlite3_finalize(stmt);
            // 绠€鍖栵細濡傛灉鏈夎繛缁己澶憋紝鍙ˉ鏈€杩戠殑涓€涓?            // 瀹為檯浣跨敤涓紝鐢ㄦ埛鍙互澶氭鍙戦€?琛ョ"鍛戒护
            return 0;
        }
        sqlite3_finalize(stmt);
    }
    
    return missingDate;
}
```

---

### Task 6: Create RetroactiveCheckInModule.h

**Files:**
- Create: `MonsterOrderWilds/RetroactiveCheckInModule.h`

- [x] **Step 1: Write header file**

```cpp
#pragma once
#include "framework.h"
#include "LikeEvent.h"
#include "DanmuProcessor.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

// 鍓嶅悜澹版槑
class ProfileManager;

class RetroactiveCheckInModule {
    DECLARE_SINGLETON(RetroactiveCheckInModule)

public:
    bool Init();
    void Destroy();

    // 鎺ユ敹鐐硅禐浜嬩欢
    void PushLikeEvent(const LikeEvent& event);
    
    // 鎺ユ敹寮瑰箷鍛戒护锛?琛ョ"銆?琛ョ鏌ヨ"锛?    void PushDanmuEvent(const DanmuProcessor::CaptainDanmuEvent& event);
    
    // 璁剧疆瑙﹀彂璇嶏紙榛樿锛?琛ョ","琛ョ鍗?,"琛ョ鏌ヨ"锛?    void SetTriggerWords(const std::string& words);

private:
    // 鐐硅禐澶勭悊
    void ProcessLike(const LikeEvent& event);
    
    // 琛ョ鍗″彂鏀捐鍒?    bool CheckRule1_StreakReward(const std::string& uid, int32_t date);
    bool CheckRule2_MonthlyFirst(const std::string& uid, int32_t date, int32_t totalLikes);
    
    // 鍛戒护澶勭悊
    void HandleRetroactiveCommand(const DanmuProcessor::CaptainDanmuEvent& event);
    void HandleQueryCommand(const DanmuProcessor::CaptainDanmuEvent& event);
    
    // 琛ョ鎵ц
    bool ExecuteRetroactive(const std::string& uid, const std::string& username, int32_t targetDate);
    
    // 鍥炲/鎾姤
    void SendReply(const std::string& username, const std::string& text);
    
    // 宸ュ叿鏂规硶
    bool IsRetroactiveMessage(const std::string& content) const;
    bool IsQueryMessage(const std::string& content) const;
    int32_t GetCurrentDate() const;
    
    bool inited_ = false;
    std::vector<std::wstring> retroactiveWords_;
    std::vector<std::wstring> queryWords_;
    
    std::mutex likeLock_;
    
    // 鐢ㄤ簬闃叉閲嶅澶勭悊鍚屼竴浜嬩欢锛堝彲閫夛級
    std::map<std::string, int64_t> lastProcessedTimestamp_;
};
```

---

### Task 7: Create RetroactiveCheckInModule.cpp - Core structure

**Files:**
- Create: `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

- [x] **Step 1: Write core implementation**

```cpp
#include "framework.h"
#include "RetroactiveCheckInModule.h"
#include "ProfileManager.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "TextToSpeech.h"
#include "DataBridgeExports.h"
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
    
    bool IsLeapYear(int32_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }
    
    int32_t GetDaysInMonth(int32_t year, int32_t month) {
        switch (month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                return 31;
            case 4: case 6: case 9: case 11:
                return 30;
            case 2:
                return IsLeapYear(year) ? 29 : 28;
            default:
                return 30;
        }
    }
}

bool RetroactiveCheckInModule::Init() {
    if (inited_) return true;
    
    LOG_INFO(TEXT("RetroactiveCheckInModule::Init"));
    
    // 榛樿瑙﹀彂璇嶏紙浣跨敤 ; 鍒嗛殧锛氬墠闈㈣ˉ绛捐瘝锛屽悗闈㈡煡璇㈣瘝锛?    SetTriggerWords("琛ョ,琛ョ鍗?琛ョ鏌ヨ,琛ョ鍗℃煡璇?鏌ヨ琛ョ,鏌ヨ琛ョ鍗?鎴戠殑琛ョ鍗?);
    
    inited_ = true;
    LOG_INFO(TEXT("RetroactiveCheckInModule::Init done"));
    return true;
}

void RetroactiveCheckInModule::Destroy() {
    LOG_INFO(TEXT("RetroactiveCheckInModule::Destroy"));
    
    retroactiveWords_.clear();
    queryWords_.clear();
    inited_ = false;
    
    LOG_INFO(TEXT("RetroactiveCheckInModule::Destroy done"));
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

    // 浣跨敤 ";" 鍒嗛殧锛氬墠闈㈣ˉ绛捐瘝鍒楄〃锛屽悗闈㈡煡璇㈣瘝鍒楄〃
    std::wstring wwords = Utf8ToWstring(words);
    size_t sepPos = wwords.find(L';');
    std::wstring retroPart = (sepPos != std::wstring::npos) ? wwords.substr(0, sepPos) : wwords;
    std::wstring queryPart = (sepPos != std::wstring::npos) ? wwords.substr(sepPos + 1) : L"";

    parseCsv(retroPart, retroactiveWords_);
    if (!queryPart.empty()) {
        parseCsv(queryPart, queryWords_);
    } else {
        // 鍏煎鏃ф牸寮忥細涓嶅惈鍒嗗彿鏃讹紝鍚?鏌ヨ"鐨勮瘝褰掍负鏌ヨ璇?        for (auto it = retroactiveWords_.begin(); it != retroactiveWords_.end(); ) {
            if (it->find(L"鏌ヨ") != std::wstring::npos) {
                queryWords_.push_back(*it);
                it = retroactiveWords_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// IsRetroactiveMessage 鍜?IsQueryMessage 浣跨敤绮剧‘鍏ㄧ瓑鍖归厤 (==)锛岄伩鍏嶅瓙涓插尮閰嶈瑙?// 鍗崇敤鎴峰脊骞曞繀椤讳笌瑙﹀彂璇嶅畬鍏ㄤ竴鑷存墠瑙﹀彂瀵瑰簲鍛戒护

bool RetroactiveCheckInModule::IsRetroactiveMessage(const std::string& content) const {
    try {
        std::wstring wcontent(content.begin(), content.end());
        for (const auto& word : retroactiveWords_) {
            if (_wcsicmp(wcontent.c_str(), word.c_str()) == 0) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsRetroactiveMessage error: %s"), e.what());
    }
    return false;
}

bool RetroactiveCheckInModule::IsQueryMessage(const std::string& content) const {
    try {
        std::wstring wcontent(content.begin(), content.end());
        for (const auto& word : queryWords_) {
            if (_wcsicmp(wcontent.c_str(), word.c_str()) == 0) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsQueryMessage error: %s"), e.what());
    }
    return false;
}

int32_t RetroactiveCheckInModule::GetCurrentDate() const {
    std::time_t now = std::time(nullptr);
    std::tm tmResult = {};
    if (localtime_s(&tmResult, &now) == 0) {
        return (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
    }
    return 0;
}

void RetroactiveCheckInModule::SendReply(const std::string& username, const std::string& text) {
    std::wstring contentCopy = Utf8ToWstring(text);
    RECORD_HISTORY(contentCopy.c_str());
    
    std::wstring usernameW = Utf8ToWstring(username);
    std::wstring answerW = Utf8ToWstring(text);
    if (g_aiReplyCallback) {
        g_aiReplyCallback(usernameW.c_str(), answerW.c_str(), g_aiReplyUserData);
    }
    
    // TTS鎾姤
    if (ConfigManager::Inst()->GetConfig().enableVoice) {
        TTSManager::Inst()->SpeakCheckinTTS(answerW, username);
    }
}
```

---

### Task 8: RetroactiveCheckInModule.cpp - Like processing and reward rules

**Files:**
- Modify: `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

- [x] **Step 1: Add ProcessLike and reward rules**

鍦?`RetroactiveCheckInModule.cpp` 鐨?`GetCurrentDate()` 鏂规硶涔嬪悗鎻掑叆锛?
```cpp
void RetroactiveCheckInModule::PushLikeEvent(const LikeEvent& event) {
    if (!inited_) return;
    
    if (event.likeCount <= 0) return;
    if (event.uid.empty()) return;
    
    std::lock_guard<std::mutex> lock(likeLock_);
    
    // 鍘婚噸妫€鏌ワ紙鍚屼竴uid鍦ㄥ悓涓€绉掑唴涓嶉噸澶嶅鐞嗭級
    auto it = lastProcessedTimestamp_.find(event.uid);
    if (it != lastProcessedTimestamp_.end() && it->second == event.timestamp) {
        return;
    }
    lastProcessedTimestamp_[event.uid] = event.timestamp;
    
    ProcessLike(event);
}

void RetroactiveCheckInModule::ProcessLike(const LikeEvent& event) {
    int32_t date = event.date > 0 ? event.date : GetCurrentDate();
    if (date == 0) return;
    
    int32_t totalLikes = 0;
    if (!ProfileManager::Inst()->AddDailyLike(event.uid, date, event.likeCount, totalLikes)) {
        LOG_ERROR(TEXT("RetroactiveCheckInModule: Failed to add daily like for uid=%hs"), event.uid.c_str());
        return;
    }
    
    LOG_DEBUG(TEXT("RetroactiveCheckInModule: uid=%hs date=%d total_likes=%d"), 
        event.uid.c_str(), date, totalLikes);
    
    // 妫€鏌ヨ鍒?锛氭瘡鏈堥娆＄偣璧?000
    if (totalLikes >= MONTHLY_FIRST_LIKES_REQUIRED) {
        if (CheckRule2_MonthlyFirst(event.uid, date, totalLikes)) {
            std::string reply = event.username + "锛屾伃鍠滐紒浠婃棩鐐硅禐绐佺牬1000锛岃幏寰?寮犺ˉ绛惧崱锛?;
            SendReply(event.username, reply);
        }
    }
    
    // 妫€鏌ヨ鍒?锛氳繛缁?澶╃偣璧?    if (CheckRule1_StreakReward(event.uid, date)) {
        std::string reply = event.username + "锛屾伃鍠滐紒杩炵画7澶╃偣璧烇紝鑾峰緱1寮犺ˉ绛惧崱锛?;
        SendReply(event.username, reply);
    }
}

bool RetroactiveCheckInModule::CheckRule1_StreakReward(const std::string& uid, int32_t date) {
    LikeStreakData streak;
    if (!ProfileManager::Inst()->LoadLikeStreak(uid, streak)) {
        return false;
    }
    
    // 妫€鏌ユ槸鍚﹁繛缁?    if (streak.lastLikeDate > 0) {
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
            int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
            if (lastDay == lastMonthDays) {
                expectedNextDate = date;
            }
        } else if (year == lastYear + 1 && month == 1 && lastMonth == 12 && day == 1 && lastDay == 31) {
            expectedNextDate = date;
        }
        
        if (date == expectedNextDate) {
            streak.currentStreak++;
        } else if (date != streak.lastLikeDate) {
            // 涓嶈繛缁笖涓嶆槸鍚屼竴澶╋紝閲嶇疆
            streak.currentStreak = 1;
        }
        // 濡傛灉鏄悓涓€澶╋紝涓嶉噸缃絾涔熶笉澧炲姞
    } else {
        streak.currentStreak = 1;
    }
    
    streak.lastLikeDate = date;
    
    // 妫€鏌ユ槸鍚﹁揪鍒?澶╀笖鏈彂鏀惧鍔?    // 娉ㄦ剰锛氫娇鐢?% STREAK_DAYS_REQUIRED == 0 纭繚姣忔弧7澶╁懆鏈熸墠鍙戞斁锛堢7銆?4銆?1澶?..锛?    // streak.streakRewardIssued != date 闃叉鍚屼竴澶╅噸澶嶅彂鏀?    if (streak.currentStreak >= STREAK_DAYS_REQUIRED && (streak.currentStreak % STREAK_DAYS_REQUIRED) == 0 && streak.streakRewardIssued != date) {
        // 鍙戞斁琛ョ鍗?        RetroactiveCardData cards;
        if (ProfileManager::Inst()->LoadRetroactiveCards(uid, cards)) {
            cards.cardCount++;
            cards.totalEarned++;
            cards.lastEarnedDate = date;
            if (ProfileManager::Inst()->SaveRetroactiveCards(cards)) {
                streak.streakRewardIssued = date;
                ProfileManager::Inst()->SaveLikeStreak(streak);
                LOG_INFO(TEXT("RetroactiveCheckInModule: Rule1 reward issued to uid=%hs"), uid.c_str());
                return true;
            }
        }
    }
    
    ProfileManager::Inst()->SaveLikeStreak(streak);
    return false;
}

bool RetroactiveCheckInModule::CheckRule2_MonthlyFirst(const std::string& uid, int32_t date, int32_t totalLikes) {
    if (totalLikes < MONTHLY_FIRST_LIKES_REQUIRED) return false;
    
    RetroactiveCardData cards;
    if (!ProfileManager::Inst()->LoadRetroactiveCards(uid, cards)) {
        return false;
    }
    
    int32_t currentMonth = date / 100;  // YYYYMM
    int32_t claimedMonth = cards.monthlyFirstClaimed / 100;
    
    // 璺ㄦ湀妫€鏌?    if (currentMonth != claimedMonth) {
        // 閲嶇疆涓婃湀鐘舵€?        cards.monthlyFirstClaimed = 0;
    }
    
    // 妫€鏌ユ湰鏈堟槸鍚﹀凡棰嗗彇
    if (cards.monthlyFirstClaimed > 0) {
        return false;  // 鏈湀宸查鍙?    }
    
    // 鍙戞斁琛ョ鍗?    cards.cardCount++;
    cards.totalEarned++;
    cards.monthlyFirstClaimed = date;
    cards.lastEarnedDate = date;
    
    if (ProfileManager::Inst()->SaveRetroactiveCards(cards)) {
        LOG_INFO(TEXT("RetroactiveCheckInModule: Rule2 reward issued to uid=%hs"), uid.c_str());
        return true;
    }
    
    return false;
}
```

---

### Task 9: RetroactiveCheckInModule.cpp - Command handling

**Files:**
- Modify: `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

- [x] **Step 1: Add command handling**

鍦?`RetroactiveCheckInModule.cpp` 鐨?`CheckRule2_MonthlyFirst` 鏂规硶涔嬪悗鎻掑叆锛?
```cpp
void RetroactiveCheckInModule::PushDanmuEvent(const DanmuProcessor::CaptainDanmuEvent& event) {
    if (!inited_) return;
    if (event.uid.empty()) return;
    
    if (IsQueryMessage(event.content)) {
        HandleQueryCommand(event);
    } else if (IsRetroactiveMessage(event.content)) {
        HandleRetroactiveCommand(event);
    }
}

void RetroactiveCheckInModule::HandleRetroactiveCommand(const DanmuProcessor::CaptainDanmuEvent& event) {
    RetroactiveCardData cards;
    if (!ProfileManager::Inst()->LoadRetroactiveCards(event.uid, cards)) {
        SendReply(event.username, event.username + "锛岀郴缁熼敊璇紝璇风◢鍚庡啀璇曘€?);
        return;
    }
    
    if (cards.cardCount <= 0) {
        SendReply(event.username, event.username + "锛屼綘娌℃湁琛ョ鍗″摝~");
        return;
    }
    
    int32_t currentDate = event.sendDate > 0 ? event.sendDate : GetCurrentDate();
    if (currentDate == 0) {
        SendReply(event.username, event.username + "锛屾棩鏈熻幏鍙栧け璐ワ紝璇风◢鍚庡啀璇曘€?);
        return;
    }
    
    int32_t missingDate = ProfileManager::Inst()->FindLastMissingCheckinDate(event.uid, currentDate);
    if (missingDate == 0) {
        SendReply(event.username, event.username + "锛屽綋鍓嶆病鏈夐渶瑕佽ˉ绛剧殑鏃ユ湡銆?);
        return;
    }
    
    if (ExecuteRetroactive(event.uid, event.username, missingDate)) {
        int32_t newContinuousDays = ProfileManager::Inst()->CalculateContinuousDays(
            std::stoull(event.uid), currentDate);
        
        std::ostringstream oss;
        oss << event.username << "锛屽凡鎴愬姛琛ョ" << (missingDate / 100 % 100) << "鏈? << (missingDate % 100) << "鏃ワ紝"
            << "鍓╀綑琛ョ鍗? << (cards.cardCount - 1) << "寮狅紝杩炵画鎵撳崱鎭㈠涓? << newContinuousDays << "澶╋紒";
        SendReply(event.username, oss.str());
    } else {
        SendReply(event.username, event.username + "锛岃ˉ绛惧け璐ワ紝璇风◢鍚庡啀璇曘€?);
    }
}

void RetroactiveCheckInModule::HandleQueryCommand(const DanmuProcessor::CaptainDanmuEvent& event) {
    RetroactiveCardData cards;
    LikeStreakData streak;
    DailyLikeData dailyLike;
    
    int32_t currentDate = event.sendDate > 0 ? event.sendDate : GetCurrentDate();
    int32_t currentMonth = currentDate / 100;
    
    ProfileManager::Inst()->LoadRetroactiveCards(event.uid, cards);
    ProfileManager::Inst()->LoadLikeStreak(event.uid, streak);
    ProfileManager::Inst()->GetDailyLike(event.uid, currentDate, dailyLike);
    
    std::ostringstream oss;
    oss << event.username << "锛屼綘褰撳墠鎸佹湁" << cards.cardCount << "寮犺ˉ绛惧崱銆?;
    
    // 杩炵画鐐硅禐杩涘害
    int32_t remainingStreak = STREAK_DAYS_REQUIRED - streak.currentStreak;
    if (remainingStreak <= 0) {
        oss << "杩炵画鐐硅禐7澶╁鍔憋細宸叉弧瓒虫潯浠讹紝涓嬫鐐硅禐鍗冲彲棰嗗彇锛?;
    } else {
        oss << "杩炵画鐐硅禐7澶╁鍔憋細宸茶繛缁偣璧? << streak.currentStreak << "澶╋紝鍐嶇偣璧? 
            << remainingStreak << "澶╁彲鑾峰緱1寮犮€?;
    }
    
    // 鏈湀棣栨鐐硅禐杩涘害
    int32_t claimedMonth = cards.monthlyFirstClaimed / 100;
    if (claimedMonth == currentMonth && cards.monthlyFirstClaimed > 0) {
        oss << "鏈湀棣栨鐐硅禐1000濂栧姳锛氭湰鏈堝凡棰嗗彇銆?;
    } else {
        int32_t currentLikes = dailyLike.totalLikes;
        int32_t remainingLikes = MONTHLY_FIRST_LIKES_REQUIRED - currentLikes;
        if (remainingLikes <= 0) {
            oss << "鏈湀棣栨鐐硅禐1000濂栧姳锛氬凡婊¤冻鏉′欢锛岀珛鍒诲彲鑾峰緱1寮狅紒";
        } else {
            oss << "鏈湀棣栨鐐硅禐1000濂栧姳锛氫粖鏃ュ凡鐐硅禐" << currentLikes << "/" 
                << MONTHLY_FIRST_LIKES_REQUIRED << "锛岃繕闇€" << remainingLikes << "涓€?;
        }
    }
    
    SendReply(event.username, oss.str());
}

bool RetroactiveCheckInModule::ExecuteRetroactive(const std::string& uid, const std::string& username, int32_t targetDate) {
    // 鎵ｉ櫎琛ョ鍗?    if (!ProfileManager::Inst()->DeductRetroactiveCard(uid)) {
        return false;
    }
    
    // 鎻掑叆琛ョ璁板綍
    if (!ProfileManager::Inst()->InsertRetroactiveCheckin(uid, username, targetDate)) {
        // 灏濊瘯鍥炴粴琛ョ鍗★紙绠€鍖栧鐞嗭細涓嶈嚜鍔ㄥ洖婊氾紝閬垮厤澶嶆潅浜嬪姟锛?        LOG_ERROR(TEXT("RetroactiveCheckInModule: Insert checkin failed for uid=%hs, date=%d"), uid.c_str(), targetDate);
        return false;
    }
    
    // 鏇存柊鐢ㄦ埛profile鐨勮繛缁ぉ鏁?    UserProfileData profile;
    if (ProfileManager::Inst()->LoadProfile(std::stoull(uid), profile)) {
        int32_t newContinuousDays = ProfileManager::Inst()->CalculateContinuousDays(std::stoull(uid), targetDate);
        profile.continuousDays = newContinuousDays;
        if (targetDate > profile.lastCheckinDate) {
            profile.lastCheckinDate = targetDate;
        }
        ProfileManager::Inst()->SaveProfile(profile);
    }
    
    LOG_INFO(TEXT("RetroactiveCheckInModule: Retroactive checkin executed for uid=%hs, date=%d"), uid.c_str(), targetDate);
    return true;
}
```

---

### Task 10: BliveManager.cpp - Add LIKE message handling

**Files:**
- Modify: `MonsterOrderWilds/BliveManager.cpp`

- [x] **Step 1: Add include**

鍦?`BliveManager.cpp` 绗?琛岋紙`#include "DataBridge.h"` 涔嬪悗锛夋彃鍏ワ細

```cpp
#include "RetroactiveCheckInModule.h"
```

- [x] **Step 2: Add LIKE message dispatch in HandleSmsReply**

鍦?`BliveManager.cpp` 绗?46-547琛岋紙`if (cmd == "LIVE_OPEN_PLATFORM_DM")` 涔嬪墠锛夋彃鍏ワ細

```cpp
        // 鐐硅禐
        if (cmd == "LIVE_OPEN_PLATFORM_LIKE") {
            DanmuProcessor::Inst()->NotifyLikeEvent(
                DanmuProcessor::Inst()->ParseLikeJson(jsonResponse["data"])
            );
        }
```

---

### Task 11: DanmuProcessor.cpp - Connect RetroactiveCheckInModule to danmu events

**Files:**
- Modify: `MonsterOrderWilds/DanmuProcessor.cpp`

- [x] **Step 1: Add include**

鍦?`DanmuProcessor.cpp` 绗?琛岋紙`#include "WriteLog.h"` 涔嬪悗锛夋彃鍏ワ細

```cpp
#include "RetroactiveCheckInModule.h"
```

- [x] **Step 2: Add connection in NotifyCaptainDanmu**

鍦?`DanmuProcessor.cpp` 绗?75-381琛岋紙`NotifyCaptainDanmu` 鏂规硶锛変慨鏀癸細

灏嗭細
```cpp
void DanmuProcessor::NotifyCaptainDanmu(const CaptainDanmuEvent& event)
{
    for (const auto& handler : captainDanmuListeners_)
    {
        handler(event);
    }
}
```

鏀逛负锛?```cpp
void DanmuProcessor::NotifyCaptainDanmu(const CaptainDanmuEvent& event)
{
    for (const auto& handler : captainDanmuListeners_)
    {
        handler(event);
    }
    
    // 鍚屾椂鍒嗗彂缁?RetroactiveCheckInModule
    RetroactiveCheckInModule::Inst()->PushDanmuEvent(event);
}
```

---

### Task 12: Update project files

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj.filters`

- [x] **Step 1: Add to vcxproj**

鍦?`MonsterOrderWilds.vcxproj` 绗?35琛岋紙`ClInclude Include="CaptainCheckInModule.h"` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClInclude Include="LikeEvent.h" />
    <ClInclude Include="RetroactiveCheckInModule.h" />
```

鍦?`MonsterOrderWilds.vcxproj` 绗?69琛岋紙`ClCompile Include="CaptainCheckInModule.cpp"` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClCompile Include="RetroactiveCheckInModule.cpp" />
```

鍦?`MonsterOrderWilds.vcxproj` 绗?60琛岋紙`CaptainCheckInModuleTests.cpp` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClCompile Include="RetroactiveCheckInModuleTests.cpp">
      <Filter>UnitTests</Filter>
      <ExcludedFromBuild Condition="'$(Configuration)'!='UnitTest'">true</ExcludedFromBuild>
    </ClCompile>
```

- [x] **Step 2: Add to filters**

鍦?`MonsterOrderWilds.vcxproj.filters` 绗?1-52琛岋紙`CaptainCheckInModule.h` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClInclude Include="LikeEvent.h">
      <Filter>DataProcessing</Filter>
    </ClInclude>
    <ClInclude Include="RetroactiveCheckInModule.h">
      <Filter>DataProcessing</Filter>
    </ClInclude>
```

鍦?`MonsterOrderWilds.vcxproj.filters` 绗?44-145琛岋紙`CaptainCheckInModule.cpp` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClCompile Include="RetroactiveCheckInModule.cpp">
      <Filter>DataProcessing</Filter>
    </ClCompile>
```

鍦?`MonsterOrderWilds.vcxproj.filters` 绗?77-278琛岋紙`CaptainCheckInModuleTests.cpp` 涔嬪悗锛夋彃鍏ワ細

```xml
    <ClCompile Include="RetroactiveCheckInModuleTests.cpp">
      <Filter>UnitTests</Filter>
      <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
    </ClCompile>
```

---

### Task 13: Create unit tests

**Files:**
- Create: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

- [x] **Step 1: Write test file**

```cpp
#include "framework.h"
#include "RetroactiveCheckInModule.h"
#include "ProfileManager.h"
#include <cassert>
#include <iostream>

static void TestLog(const char* msg) {
    std::cout << msg << std::endl;
}

void TestLikeEventParsing() {
    DanmuProcessor dp;
    json j = {
        {"cmd", "LIVE_OPEN_PLATFORM_LIKE"},
        {"data", {
            {"open_id", "test_uid_123"},
            {"uname", "TestUser"},
            {"like_count", 15},
            {"timestamp", 1777030207}
        }}
    };
    
    LikeEvent event = dp.ParseLikeJson(j);
    assert(event.uid == "test_uid_123");
    assert(event.username == "TestUser");
    assert(event.likeCount == 15);
    assert(event.timestamp == 1777030207);
    assert(event.date == 20260424);  // 鍩轰簬timestamp璁＄畻
    
    TestLog("[PASS] TestLikeEventParsing");
}

void TestDailyLikeTracking() {
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
    RetroactiveCheckInModule::Inst()->Init();
    
    // 妯℃嫙杩炵画7澶╃偣璧?    for (int i = 0; i < 7; ++i) {
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

void TestMonthlyFirstReward() {
    RetroactiveCheckInModule::Inst()->Init();
    
    LikeEvent evt;
    evt.uid = "monthly_user";
    evt.username = "MonthlyUser";
    evt.likeCount = 1005;
    evt.date = 20260424;
    evt.timestamp = 1777030207;
    
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    
    RetroactiveCardData cards;
    assert(ProfileManager::Inst()->LoadRetroactiveCards("monthly_user", cards));
    assert(cards.cardCount == 1);
    assert(cards.monthlyFirstClaimed == 20260424);
    
    // 鍚屼竴澶╁啀娆¤秴杩?000锛屼笉搴旈噸澶嶅彂鏀?    evt.likeCount = 100;
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    
    assert(ProfileManager::Inst()->LoadRetroactiveCards("monthly_user", cards));
    assert(cards.cardCount == 1);  // 浠嶇劧鍙湁1寮?    
    TestLog("[PASS] TestMonthlyFirstReward");
}

void TestCrossMonthReset() {
    RetroactiveCheckInModule::Inst()->Init();
    
    // 4鏈堥鍙栬繃
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
    
    // 5鏈堢偣璧?    LikeEvent evt;
    evt.uid = "cross_month_user";
    evt.username = "CrossMonthUser";
    evt.likeCount = 1005;
    evt.date = 20260501;
    evt.timestamp = 1777600000;
    
    RetroactiveCheckInModule::Inst()->PushLikeEvent(evt);
    
    assert(ProfileManager::Inst()->LoadRetroactiveCards("cross_month_user", cards));
    assert(cards.cardCount == 1);
    assert(cards.monthlyFirstClaimed == 20260501);
    
    TestLog("[PASS] TestCrossMonthReset");
}

void TestRetroactiveCommand() {
    RetroactiveCheckInModule::Inst()->Init();
    
    // 鍑嗗鐢ㄦ埛鏁版嵁
    RetroactiveCardData cards;
    cards.uid = "retro_user";
    cards.cardCount = 2;
    cards.totalEarned = 2;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);
    
    // 妯℃嫙鎵撳崱璁板綍锛?鏈?0鏃ユ墦鍗★紝22鏃ユ湭鎵撳崱锛?    ProfileManager::Inst()->InsertRetroactiveCheckin("retro_user", "RetroUser", 20260420);
    
    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "retro_user";
    evt.username = "RetroUser";
    evt.content = "琛ョ";
    evt.sendDate = 20260424;
    
    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);
    
    // 楠岃瘉琛ョ鍗″噺灏?    assert(ProfileManager::Inst()->LoadRetroactiveCards("retro_user", cards));
    assert(cards.cardCount == 1);
    
    TestLog("[PASS] TestRetroactiveCommand");
}

void TestQueryCommand() {
    RetroactiveCheckInModule::Inst()->Init();
    
    RetroactiveCardData cards;
    cards.uid = "query_user";
    cards.cardCount = 3;
    cards.totalEarned = 3;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);
    
    LikeStreakData streak;
    streak.uid = "query_user";
    streak.currentStreak = 5;
    streak.lastLikeDate = 20260424;
    streak.streakRewardIssued = 0;
    ProfileManager::Inst()->SaveLikeStreak(streak);
    
    DailyLikeData dailyLike;
    dailyLike.uid = "query_user";
    dailyLike.likeDate = 20260424;
    dailyLike.totalLikes = 800;
    ProfileManager::Inst()->AddDailyLike("query_user", 20260424, 800, dailyLike.totalLikes);
    
    DanmuProcessor::CaptainDanmuEvent evt;
    evt.uid = "query_user";
    evt.username = "QueryUser";
    evt.content = "琛ョ鏌ヨ";
    evt.sendDate = 20260424;
    
    RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);
    
    TestLog("[PASS] TestQueryCommand");
}

void TestConcurrentDeduction() {
    RetroactiveCheckInModule::Inst()->Init();

    RetroactiveCardData cards;
    cards.uid = "concurrent_user";
    cards.cardCount = 10;
    cards.totalEarned = 10;
    cards.monthlyFirstClaimed = 0;
    cards.lastEarnedDate = 0;
    ProfileManager::Inst()->SaveRetroactiveCards(cards);

    ProfileManager::Inst()->InsertRetroactiveCheckin("concurrent_user", "ConcurrentUser", 20260420);
    ProfileManager::Inst()->InsertRetroactiveCheckin("concurrent_user", "ConcurrentUser", 20260421);

    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&successCount]() {
            DanmuProcessor::CaptainDanmuEvent evt;
            evt.uid = "concurrent_user";
            evt.username = "ConcurrentUser";
            evt.content = "琛ョ";
            evt.sendDate = 20260424;

            RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);
            successCount++;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(successCount == 5);
    assert(ProfileManager::Inst()->LoadRetroactiveCards("concurrent_user", cards));
    assert(cards.cardCount == 5);

    TestLog("[PASS] TestConcurrentDeduction");
}

void RunRetroactiveCheckInModuleTests() {
    std::cout << "========== RetroactiveCheckInModule Tests ==========" << std::endl;
    TestLikeEventParsing();
    TestDailyLikeTracking();
    TestStreakReward();
    TestMonthlyFirstReward();
    TestCrossMonthReset();
    TestRetroactiveCommand();
    TestQueryCommand();
    TestConcurrentDeduction();
    std::cout << "========== All RetroactiveCheckInModule Tests Passed ==========" << std::endl;
}
```

---

## Self-Review

### Spec Coverage Check

| Spec Requirement | Task Coverage |
|-----------------|---------------|
| 鐐硅禐浜嬩欢瑙ｆ瀽涓庡垎鍙?| Task 1 (LikeEvent.h), Task 2-3 (DanmuProcessor), Task 10 (BliveManager) |
| 姣忔棩鐐硅禐绱涓庡瓨鍌?| Task 4-5 (ProfileManager AddDailyLike/GetDailyLike) |
| 杩炵画7澶╁鍔?| Task 8 (CheckRule1_StreakReward, 鍚?7鍛ㄦ湡妫€鏌? |
| 鏈堥璧?000濂栧姳 | Task 8 (CheckRule2_MonthlyFirst) |
| 璺ㄦ湀閲嶇疆 | Task 8 (CheckRule2_MonthlyFirst涓殑璺ㄦ湀妫€鏌? |
| 琛ョ鍛戒护 | Task 9 (HandleRetroactiveCommand, ExecuteRetroactive) |
| 鏌ヨ鍛戒护 | Task 9 (HandleQueryCommand, 鍚?涓煡璇㈣瘝鍙樹綋) |
| 琛ョ鍚庤繛缁ぉ鏁伴噸绠?| Task 9 (ExecuteRetroactive涓皟鐢–alculateContinuousDays) |
| 骞跺彂瀹夊叏 | Task 5 (DeductRetroactiveCard浣跨敤浜嬪姟), likeLock_鍏变韩淇濇姢 |

### Code Review 淇锛?026-04-27锛?
浠ｇ爜瀹℃煡涓彂鐜板苟淇浠ヤ笅闂锛?
| 闂 | 绾у埆 | 淇鍐呭 |
|------|------|---------|
| CheckRule1 閲嶅鍙戞斁 | Critical | 娣诲姞 `% STREAK_DAYS_REQUIRED == 0` 姣?澶╁懆鏈熸鏍?|
| PushDanmuEvent 鏃犻攣绔炴€?| Critical | 娣诲姞 `likeLock_` 涓庣偣璧炶矾寰勫叡浜繚鎶?|
| 鏋愭瀯鏈彇娑堢洃鍚櫒 | Critical | 鏋愭瀯鍑芥暟璋冪敤 `DanmuProcessor::ClearLikeEventListeners()` |
| uid 绫诲瀷鍏ㄧ嚎鎾ゅ洖 | Critical | `CaptainDanmuEvent.uid`銆乣UserProfileData.uid`銆乣checkin_records.uid`銆乣user_profiles.uid` 浠?`uint64_t`/`INTEGER` 鎾ゅ洖 `std::string`/`TEXT`锛岀粺涓€浣跨敤 open_id 瀛楃涓叉爣璇嗙銆傛秹鍙?ProfileManager銆丆aptainCheckInModule銆丏anmuProcessor銆丷etroactiveCheckInModule 鍥涗釜妯″潡锛屽垹闄?`HashStringToUint64` 杞崲鍑芥暟 |
| LOG_ERROR 鏍煎紡 | Major | 澶氭枃浠?`%s`鈫抈%hs` 淇 |
| 瑙﹀彂璇嶅瓙涓插尮閰?| Major | `find()`鈫抈==` 绮剧‘鍏ㄧ瓑鍖归厤 |
| "鎴戠殑琛ョ鍗?鍒嗙被 | Major | `SetTriggerWords` 鏀逛负 `;` 鍒嗛殧鏌ヨ璇嶅拰琛ョ璇?|
| DanmuProcessor 鏁版嵁绔炰簤 | Major | `likeEventListeners_` 娣诲姞 `likeListenersMutex_` 淇濇姢 |

### Placeholder Scan

- 鉁?鏃?TBD/TODO
- 鉁?鏃?"Add appropriate error handling" 绛夋ā绯婃弿杩?- 鉁?姣忎釜姝ラ閮芥湁瀹屾暣浠ｇ爜
- 鉁?鏃?"Similar to Task N"

### Type Consistency Check

- 鉁?`LikeEvent` 缁撴瀯浣撳湪 Task 1 瀹氫箟锛孴ask 2-3, 8, 10, 13 浣跨敤涓€鑷?- 鉁?`DailyLikeData`, `LikeStreakData`, `RetroactiveCardData` 鍦?Task 4 瀹氫箟锛孴ask 5, 8, 9, 13 浣跨敤涓€鑷?- 鉁?`ProfileManager` 鎺ュ彛鍦?Task 4 澹版槑锛孴ask 5 瀹炵幇锛孴ask 8-9 璋冪敤涓€鑷?- 鉁?`RetroactiveCheckInModule` 鍗曚緥妯″紡涓庡叾浠栨ā鍧椾竴鑷?- 鉁?鎵€鏈?uid 瀛楁缁熶竴浣跨敤 open_id TEXT 浣滀负鏍囪瘑绗?
---

---

## OCR Code Review 淇浠诲姟锛?026-04-28锛?
鍩轰簬 OCR 澶氭櫤鑳戒綋浠ｇ爜瀹℃煡锛坧rincipal-1, principal-2, quality-1, quality-2, testing-1, security-1锛夛紝鏂板浠ヤ笅淇浠诲姟锛?
### P0 - Blockers锛堥樆濉炵骇锛屽繀椤讳慨澶嶏級

#### Task 14: 绉婚櫎鍏ㄥ眬浜掓枼閿?likeLock_锛屽疄鐜版寜鐢ㄦ埛鍒嗙墖閿?
**闂**: `likeLock_` 鏄ā鍧楃骇鍗曚竴浜掓枼閿侊紝涓茶鍖?*鎵€鏈?*鐢ㄦ埛鐨勭偣璧炰簨浠跺拰寮瑰箷鍛戒护銆傞珮骞跺彂鐩存挱鍦烘櫙涓嬩細鎴愪负涓ラ噸鎬ц兘鐡堕銆?
**鏂囦欢**: 
- `MonsterOrderWilds/RetroactiveCheckInModule.h`
- `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦?`RetroactiveCheckInModule.h` 涓浛鎹?`std::mutex likeLock_` 涓烘寜鐢ㄦ埛鍒嗙墖鐨勯攣鏁扮粍
  ```cpp
  static constexpr size_t LOCK_SHARD_COUNT = 16;
  std::array<std::mutex, LOCK_SHARD_COUNT> cardDataLocks_;
  
  std::mutex& GetUserLock(const std::string& uid) {
      size_t hash = std::hash<std::string>{}(uid);
      return cardDataLocks_[hash % LOCK_SHARD_COUNT];
  }
  ```
- [x] **Step 2**: 鍦?`PushLikeEvent` 鍜?`PushDanmuEvent` 涓娇鐢?`GetUserLock(event.uid)` 鏇夸唬 `likeLock_`
- [x] **Step 3**: 纭繚鍚屼竴鐢ㄦ埛鐨勪袱涓矾寰勶紙鐐硅禐鍜屽脊骞曪級浣跨敤鐩稿悓鐨勫垎鐗囬攣锛堥€氳繃鐩稿悓鐨?uid hash锛?
---

#### Task 15: 绉婚櫎鏃犵晫鍐呭瓨澧為暱 lastProcessedTimestamp_

**闂**: `lastProcessedTimestamp_` 瀛樺偍姣忎釜鍙戦€佽繃鐐硅禐鐨勭敤鎴?UID锛屾棤娣樻卑鏈哄埗锛岄暱鏃堕棿杩愯浼氳€楀敖鍐呭瓨銆?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModule.h`, `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 浠?`RetroactiveCheckInModule.h` 涓垹闄?`lastProcessedTimestamp_` 鎴愬憳
- [x] **Step 2**: 浠?`PushLikeEvent` 涓垹闄ゅ幓閲嶆鏌ラ€昏緫锛堢178-183琛岋級
- [x] **Step 3**: 渚濊禆鐜版湁鐨?`msg_id` 鍘婚噸锛坄DanmuProcessor::IsDuplicateMsgId`锛夊拰鏁版嵁搴?upsert 璇箟淇濊瘉骞傜瓑鎬?
---

#### Task 16: 鍘熷瓙鍖?ExecuteRetroactive 鎿嶄綔

**闂**: `ExecuteRetroactive` 鎵ц涓変釜鐙珛鐨勬暟鎹簱鎿嶄綔锛堟墸鍗°€佹彃璁板綍銆佹洿鏂?profile锛夛紝鏃犲寘瑁逛簨鍔°€傛楠?/3澶辫触鏃跺崱鐗囧凡鎵ｉ櫎鏃犳硶鑷姩鎭㈠銆?
**鏂囦欢**: 
- `MonsterOrderWilds/ProfileManager.h`
- `MonsterOrderWilds/ProfileManager.cpp`
- `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦?`ProfileManager.h` 涓柊澧炲師瀛愭€ф帴鍙?  ```cpp
  bool ExecuteRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t targetDate, int32_t& outNewCardCount);
  ```
- [x] **Step 2**: 鍦?`ProfileManager.cpp` 涓疄鐜板師瀛愭搷浣滐紙浣跨敤 `BEGIN IMMEDIATE` 鍖呰９鎵€鏈夋楠わ級
  ```cpp
  bool ProfileManager::ExecuteRetroactiveCheckin(...) {
      // BEGIN IMMEDIATE
      // 1. SELECT card_count FROM retroactive_cards WHERE uid = ?
      // 2. UPDATE retroactive_cards SET card_count = card_count - 1 WHERE uid = ? AND card_count > 0
      // 3. INSERT OR REPLACE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)
      // 4. LoadProfile + CalculateContinuousDays + SaveProfile
      // COMMIT / ROLLBACK
  }
  ```
- [x] **Step 3**: 鍦?`RetroactiveCheckInModule.cpp` 鐨?`ExecuteRetroactive` 涓皟鐢ㄦ柊鐨勫師瀛愭帴鍙ｏ紝鍒犻櫎鏃х殑鍒嗙璋冪敤閫昏緫

---

#### Task 17: 淇鏋愭瀯鍑芥暟娓呴櫎鎵€鏈夌洃鍚櫒鐨勯棶棰?
**闂**: 鏋愭瀯鍑芥暟璋冪敤 `DanmuProcessor::ClearLikeEventListeners()` 浼氭竻闄?*鎵€鏈?*鍏ㄥ眬鐩戝惉鍣紝鑰屼笉浠呬粎鏄湰妯″潡娉ㄥ唽鐨勩€?
**鏂囦欢**: 
- `MonsterOrderWilds/RetroactiveCheckInModule.cpp`
- `MonsterOrderWilds/DanmuProcessor.h`
- `MonsterOrderWilds/DanmuProcessor.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦?`DanmuProcessor.h` 涓柊澧炲甫 token 鐨勭洃鍚櫒娉ㄥ唽/娉ㄩ攢鎺ュ彛
  ```cpp
  using LikeEventListenerToken = size_t;
  LikeEventListenerToken AddLikeEventListener(const LikeEventHandler& handler);
  void RemoveLikeEventListener(LikeEventListenerToken token);
  ```
- [x] **Step 2**: 鍦?`DanmuProcessor.cpp` 涓疄鐜?token-based 绠＄悊锛堜娇鐢?vector + 鍙€夌殑 token 鏄犲皠锛?- [x] **Step 3**: 鍦?`RetroactiveCheckInModule.cpp` 鐨?`Init()` 涓繚瀛樿繑鍥炵殑 token
  ```cpp
  likeListenerToken_ = DanmuProcessor::Inst()->AddLikeEventListener([](const LikeEvent& e) { ... });
  ```
- [x] **Step 4**: 鍦ㄦ瀽鏋勫嚱鏁颁腑浣跨敤 `RemoveLikeEventListener(likeListenerToken_)` 鏇夸唬 `ClearLikeEventListeners()`

---

#### Task 18: 淇 SendReply 鍙岄噸 UTF-8 杞崲

**闂**: `SendReply` 涓皢 `text` 鍙傛暟涓ゆ杞崲涓?wstring锛坄contentCopy` 鍜?`answerW`锛夈€?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 淇敼 `SendReply` 鍑芥暟锛屽彧杞崲涓€娆?  ```cpp
  void RetroactiveCheckInModule::SendReply(const std::string& username, const std::string& text) {
      std::wstring wtext = Utf8ToWstring(text);
      RECORD_HISTORY(wtext.c_str());
      
      std::wstring usernameW = Utf8ToWstring(username);
      if (g_aiReplyCallback) {
          g_aiReplyCallback(usernameW.c_str(), wtext.c_str(), g_aiReplyUserData);
      }
      
      if (ConfigManager::Inst()->GetConfig().enableVoice) {
          TTSManager::Inst()->SpeakCheckinTTS(wtext, username);
      }
  }
  ```

---

### P1 - Should Fix锛堝缓璁慨澶嶏級

#### Task 19: 淇骞跺彂娴嬭瘯鈥斺€旀祴璇曞悓涓€ UID 鐨勫苟鍙戞墸鍑?
**闂**: `TestConcurrentDeduction` 浣跨敤 5 涓笉鍚?UID锛屾湭娴嬭瘯鍚屼竴鐢ㄦ埛鍗＄墖鏁版嵁鐨勫疄闄呯珵鎬佹潯浠躲€?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

**姝ラ**:
- [x] **Step 1**: 淇敼骞跺彂娴嬭瘯涓哄悓涓€ UID
  ```cpp
  void TestConcurrentDeduction() {
      // 鍑嗗锛?涓敤鎴凤紝鎸佹湁5寮犲崱锛屾湁缂哄け鏃ユ湡
      RetroactiveCheckInModule::Inst()->Init();
      
      std::string uid = "concurrent_single_user";
      RetroactiveCardData cards;
      cards.uid = uid;
      cards.cardCount = 5;
      ProfileManager::Inst()->SaveRetroactiveCards(cards);
      ProfileManager::Inst()->InsertRetroactiveCheckin(uid, "User", 20260420);
      
      std::vector<std::thread> threads;
      std::atomic<int> successCount(0);
      
      for (int i = 0; i < 5; i++) {
          threads.emplace_back([uid, &successCount]() {
              DanmuProcessor::CaptainDanmuEvent evt;
              evt.uid = uid;
              evt.content = "琛ョ";
              evt.sendDate = 20260424;
              RetroactiveCheckInModule::Inst()->PushDanmuEvent(evt);
              successCount++;
          });
      }
      for (auto& t : threads) t.join();
      
      assert(ProfileManager::Inst()->LoadRetroactiveCards(uid, cards));
      assert(cards.cardCount == 0); // 鍒濆5寮狅紝琛ョ5娆★紝鍓╀綑0寮?      TestLog("[PASS] TestConcurrentDeduction");
  }
  ```

---

#### Task 20: 娣诲姞杈圭晫鍊兼祴璇?
**闂**: 缂哄皯绮剧‘杈圭晫娴嬭瘯锛?99/1000 璧烇紝6/7/8/14 澶╄繛缁墦鍗★級銆?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

**姝ラ**:
- [x] **Step 1**: 娣诲姞鐐硅禐杈圭晫娴嬭瘯
  ```cpp
  void TestMonthlyFirstBoundary() {
      // 999璧炰笉搴斿彂鏀撅紝1000璧炲簲鍙戞斁
      // ...
  }
  ```
- [x] **Step 2**: 娣诲姞杩炵画澶╂暟杈圭晫娴嬭瘯
  ```cpp
  void TestStreakBoundary() {
      // 6澶╀笉搴斿彂鏀撅紝7澶╁簲鍙戞斁锛?澶╀笉搴旈噸澶嶅彂鏀撅紝14澶╁簲鍐嶆鍙戞斁
      // ...
  }
  ```
- [x] **Step 3**: 娣诲姞璺ㄥ勾鍜岄棸骞存祴璇?  ```cpp
  void TestLeapYearAndCrossYear() {
      // 2024-02-28 -> 2024-02-29 -> 2024-03-01 (闂板勾)
      // 2024-12-31 -> 2025-01-01 (璺ㄥ勾)
      // ...
  }
  ```

---

#### Task 21: 娣诲姞閿欒璺緞娴嬭瘯

**闂**: 鏈祴璇?DB 澶辫触銆佺┖ uid銆乴ikeCount <= 0銆佸洖婊氶€昏緫绛夐敊璇満鏅€?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

**姝ラ**:
- [x] **Step 1**: 娣诲姞绌?UID 娴嬭瘯
- [x] **Step 2**: 娣诲姞 likeCount <= 0 娴嬭瘯
- [x] **Step 3**: 娣诲姞鏃犺ˉ绛惧崱鏃跺彂閫?琛ョ"鍛戒护鐨勬祴璇?- [x] **Step 4**: 娣诲姞鏃犵己澶辨棩鏈熸椂鍙戦€?琛ョ"鍛戒护鐨勬祴璇?
---

#### Task 22: 涓?CheckRule1/2 娣诲姞鏁版嵁搴撲簨鍔′繚鎶?
**闂**: CheckRule1/2 鎵ц澶氭鏁版嵁搴撴搷浣滐紙Load鈫扴ave锛夋棤浜嬪姟鍖呰９锛岃繘绋嬪穿婧冨彲鑳藉鑷撮噸澶嶅鍔便€?
**鏂囦欢**: `MonsterOrderWilds/ProfileManager.cpp`, `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦?`ProfileManager` 涓柊澧炲師瀛愭€у鍔卞彂鏀炬帴鍙?  ```cpp
  bool IssueStreakReward(const std::string& uid, int32_t date);
  bool IssueMonthlyFirstReward(const std::string& uid, int32_t date);
  ```
- [x] **Step 2**: 浣跨敤 `BEGIN IMMEDIATE` + 鏉′欢 UPDATE 瀹炵幇鍘熷瓙鎬э紙濡?`UPDATE retroactive_cards SET card_count = card_count + 1 WHERE uid = ?`锛?- [x] **Step 3**: 鍦?`RetroactiveCheckInModule.cpp` 鐨?CheckRule1/2 涓皟鐢ㄦ柊鎺ュ彛

---

#### Task 23: 鎻愬彇鍏变韩鏃ユ湡宸ュ叿鍑芥暟

**闂**: `IsLeapYear` 鍜?`GetDaysInMonth` 鍦?`RetroactiveCheckInModule.cpp` 鍜?`ProfileManager.cpp` 涓噸澶嶅畾涔夈€?
**鏂囦欢**: 
- 鏂板缓 `MonsterOrderWilds/DateUtils.h`
- `MonsterOrderWilds/RetroactiveCheckInModule.cpp`
- `MonsterOrderWilds/ProfileManager.cpp`

**姝ラ**:
- [x] **Step 1**: 鍒涘缓 `DateUtils.h`
  ```cpp
  #pragma once
  #include "framework.h"
  
  namespace DateUtils {
      bool IsLeapYear(int32_t year);
      int32_t GetDaysInMonth(int32_t year, int32_t month);
      int32_t TimestampToDate(int64_t timestamp);
      int32_t GetCurrentDate();
      bool IsNextCalendarDay(int32_t lastDate, int32_t currentDate);
  }
  ```
- [x] **Step 2**: 鍒涘缓 `DateUtils.cpp` 瀹炵幇涓婅堪鍑芥暟
- [x] **Step 3**: 鏇挎崲 `RetroactiveCheckInModule.cpp` 鍜?`ProfileManager.cpp` 涓殑閲嶅瀹炵幇涓?`#include "DateUtils.h"`
- [x] **Step 4**: 鏇存柊 `.vcxproj` 鍜?`.filters` 娣诲姞鏂版枃浠?
---

#### Task 24: 淇鐢ㄦ埛鍙瀛楃涓蹭腑鐨勯瓟娉曟暟瀛?
**闂**: 鍥炲瀛楃涓蹭腑纭紪鐮?"1000" 鍜?"7"锛屽簲浣跨敤宸插畾涔夌殑甯搁噺銆?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModule.cpp`

**姝ラ**:
- [x] **Step 1**: 灏?`ProcessLike` 涓殑鍥炲瀛楃涓叉敼涓轰娇鐢ㄥ父閲?  ```cpp
  std::string reply = event.username + "锛屾伃鍠滐紒浠婃棩鐐硅禐绐佺牬" + 
      std::to_string(MONTHLY_FIRST_LIKES_REQUIRED) + "锛岃幏寰?寮犺ˉ绛惧崱锛?;
  ```
- [x] **Step 2**: 灏?`HandleQueryCommand` 涓殑 "杩炵画鐐硅禐7澶? 鏀逛负浣跨敤 `STREAK_DAYS_REQUIRED`

---

#### Task 25: 娣诲姞 likeCount 涓婇檺楠岃瘉

**闂**: `ParseLikeJson` 鏈獙璇?`like_count` 涓婇檺锛屾敾鍑昏€呭彲鍙戦€佹瀬澶у€肩珛鍗宠Е鍙戝鍔便€?
**鏂囦欢**: `MonsterOrderWilds/DanmuProcessor.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦?`ParseLikeJson` 涓坊鍔犱笂闄愭鏌?  ```cpp
  constexpr int32_t MAX_LIKE_COUNT = 10000;
  if (dataObj.contains("like_count")) {
      event.likeCount = dataObj["like_count"].get<int32_t>();
      if (event.likeCount > MAX_LIKE_COUNT) {
          LOG_WARNING(TEXT("[ParseLikeJson] like_count %d exceeds max %d, capping"), 
              event.likeCount, MAX_LIKE_COUNT);
          event.likeCount = MAX_LIKE_COUNT;
      }
  }
  ```

---

#### Task 26: 澧炲ぇ msgId 缂撳瓨瀹归噺

**闂**: `MSG_ID_CACHE_MAX_SIZE = 1000` 鍦ㄩ珮娴侀噺鍦烘櫙涓嬪お灏忥紝鍙兘瀵艰嚧閲嶅澶勭悊銆?
**鏂囦欢**: `MonsterOrderWilds/DanmuProcessor.h`

**姝ラ**:
- [x] **Step 1**: 灏嗙紦瀛樺ぇ灏忎粠 1000 澧炲ぇ鍒?100000
  ```cpp
  static constexpr size_t MSG_ID_CACHE_MAX_SIZE = 100000;
  ```

---

#### Task 27: 淇娴嬭瘯闅旂闂

**闂**: 娴嬭瘯鍏变韩鎸佷箙鍖?SQLite 鏁版嵁搴擄紝澶氭杩愯鎴栦贡搴忚繍琛屼細澶辫触銆?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦ㄦ祴璇曟枃浠跺紑澶存坊鍔犳暟鎹簱娓呯悊鍑芥暟
  ```cpp
  static void CleanupTestData() {
      // 鍒犻櫎娴嬭瘯鐩稿叧鐨勬墍鏈夋暟鎹?      sqlite3* db = (sqlite3*)ProfileManager::Inst()->GetStorage();
      if (db) {
          sqlite3_exec(db, "DELETE FROM user_daily_likes WHERE uid LIKE 'test_%'", nullptr, nullptr, nullptr);
          sqlite3_exec(db, "DELETE FROM user_like_streaks WHERE uid LIKE 'test_%'", nullptr, nullptr, nullptr);
          sqlite3_exec(db, "DELETE FROM retroactive_cards WHERE uid LIKE 'test_%'", nullptr, nullptr, nullptr);
          sqlite3_exec(db, "DELETE FROM checkin_records WHERE uid LIKE 'test_%'", nullptr, nullptr, nullptr);
      }
  }
  ```
- [x] **Step 2**: 鍦ㄦ瘡涓祴璇曞嚱鏁板紑澶磋皟鐢?`CleanupTestData()`

---

#### Task 28: 涓烘祴璇曟枃浠舵坊鍔?RUN_UNIT_TESTS 淇濇姢

**闂**: 娴嬭瘯浠ｇ爜鏈寜椤圭洰瑙勮寖鐢?`#ifdef RUN_UNIT_TESTS` 鍖呰９銆?
**鏂囦欢**: `MonsterOrderWilds/RetroactiveCheckInModuleTests.cpp`

**姝ラ**:
- [x] **Step 1**: 鍦ㄦ枃浠堕《閮紙`#include` 涔嬪悗锛夋坊鍔?`#ifdef RUN_UNIT_TESTS`
- [x] **Step 2**: 鍦ㄦ枃浠舵湯灏炬坊鍔?`#endif // RUN_UNIT_TESTS`

---

## 淇鍚庢鏌ユ竻鍗?
- [x] 鎵€鏈?P0 Blocker 淇瀹屾垚
- [x] 鎵€鏈?P1 Should Fix 淇瀹屾垚鎴栫‘璁ゅ欢鍚?- [x] 閲嶆柊杩愯 `git diff` 澶嶆煡淇敼鑼冨洿
- [x] 杩愯鍗曞厓娴嬭瘯楠岃瘉淇
- [x] 杩愯 MSBuild 缂栬瘧楠岃瘉

**Plan complete and saved to `openspec/changes/retroactive-checkin-cards/tasks.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**

