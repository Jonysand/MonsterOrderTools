#include "framework.h"
#include "ProfileManager.h"
#include "WriteLog.h"
#include "DateUtils.h"
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <limits>
#include <sys/stat.h>

#ifdef _WIN32
#include <Windows.h>
#include <sqlite3.h>
#endif

DEFINE_SINGLETON(ProfileManager)

namespace {
    int64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    constexpr int32_t CHECKIN_RETRY_MAX = 3;
    constexpr int32_t CHECKIN_RETRY_DELAY_MS = 500;

    bool InsertCheckinRecordWithRetry(sqlite3* db, const std::string& uid, int32_t checkinDate, int64_t timestamp, const std::string& username) {
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?) ON CONFLICT(uid, checkin_date) DO UPDATE SET created_at = excluded.created_at, username = excluded.username WHERE checkin_records.created_at != excluded.created_at";

        for (int32_t retry = 0; retry < CHECKIN_RETRY_MAX; ++retry) {
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                LOG_ERROR(TEXT("ProfileManager: Failed to prepare checkin insert (attempt %d/%d): %hs"),
                    retry + 1, CHECKIN_RETRY_MAX, sqlite3_errmsg(db));
                if (stmt) {
                    sqlite3_finalize(stmt);
                    stmt = nullptr;
                }
                if (retry < CHECKIN_RETRY_MAX - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(CHECKIN_RETRY_DELAY_MS));
                }
                continue;
            }

            sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, checkinDate);
            sqlite3_bind_int64(stmt, 3, timestamp);
            sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_TRANSIENT);

            int stepResult = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            stmt = nullptr;

            if (stepResult == SQLITE_DONE) {
                return true;
            }

            LOG_ERROR(TEXT("ProfileManager: Failed to insert checkin record (attempt %d/%d): %hs, step=%d"),
                retry + 1, CHECKIN_RETRY_MAX, sqlite3_errmsg(db), stepResult);

            if (retry < CHECKIN_RETRY_MAX - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(CHECKIN_RETRY_DELAY_MS));
            }
        }

        LOG_ERROR(TEXT("ProfileManager: Insert checkin record failed after %d attempts for uid=%hs, date=%d"),
            CHECKIN_RETRY_MAX, uid.c_str(), checkinDate);
        return false;
    }
}

std::string ProfileManager::GetDbPath() const {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);

    std::string configDir = exeDir + "\\MonsterOrderWilds_configs";
    CreateDirectoryA(configDir.c_str(), NULL);
    return configDir + "\\captain_profiles.db";
}

bool ProfileManager::Init() {
    if (storage_) return true;

    dbPath_ = GetDbPath();
    LOG_INFO(TEXT("ProfileManager::Init, db path: %hs"), dbPath_.c_str());

    sqlite3* db = nullptr;
    int result = sqlite3_open(dbPath_.c_str(), &db);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to open database: %hs"), sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    const char* createUserProfilesSql = 
        "CREATE TABLE IF NOT EXISTS user_profiles ("
        "uid TEXT PRIMARY KEY,"
        "username TEXT NOT NULL,"
        "last_checkin_date INTEGER DEFAULT 0,"
        "continuous_days INTEGER DEFAULT 0,"
        "cumulative_days INTEGER DEFAULT 0,"
        "last_danmu_timestamp INTEGER DEFAULT 0,"
        "created_at INTEGER DEFAULT 0,"
        "updated_at INTEGER DEFAULT 0,"
        "keywords_json TEXT DEFAULT '[]',"
        "danmu_history_json TEXT DEFAULT '[]'"
        ")";

    char* errMsg = nullptr;
    result = sqlite3_exec(db, createUserProfilesSql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to create user_profiles table: %hs"), errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    const char* createCheckinRecordsSql = 
        "CREATE TABLE IF NOT EXISTS checkin_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "uid TEXT NOT NULL,"
        "checkin_date INTEGER NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "username TEXT,"
        "UNIQUE(uid, checkin_date)"
        ")";

    result = sqlite3_exec(db, createCheckinRecordsSql, nullptr, nullptr, &errMsg);
    if (result != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to create checkin_records table: %hs"), errMsg);
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

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

    // 数据库迁移：检查并添加 cumulative_days 列
    const char* checkColumnSql = "PRAGMA table_info(user_profiles)";
    sqlite3_stmt* columnStmt = nullptr;
    bool hasCumulativeDays = false;
    if (sqlite3_prepare_v2(db, checkColumnSql, -1, &columnStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(columnStmt) == SQLITE_ROW) {
            const char* colName = (const char*)sqlite3_column_text(columnStmt, 1);
            if (colName && strcmp(colName, "cumulative_days") == 0) {
                hasCumulativeDays = true;
                break;
            }
        }
        sqlite3_finalize(columnStmt);
    }
    if (!hasCumulativeDays) {
        const char* alterSql = "ALTER TABLE user_profiles ADD COLUMN cumulative_days INTEGER DEFAULT 0";
        result = sqlite3_exec(db, alterSql, nullptr, nullptr, &errMsg);
        if (result != SQLITE_OK) {
            LOG_WARNING(TEXT("ProfileManager: Failed to add cumulative_days column: %hs"), errMsg);
            sqlite3_free(errMsg);
            errMsg = nullptr;
        } else {
            LOG_INFO(TEXT("ProfileManager: Added cumulative_days column to user_profiles"));
        }
    }

    storage_ = (void*)db;

    LOG_INFO(TEXT("ProfileManager::Init success"));
    return true;
}

bool ProfileManager::AddDailyLike(const std::string& uid, int32_t likeDate, int32_t likeCount, int32_t& outTotalLikes) {
    if (!storage_) return false;
    if (likeCount <= 0) return false;

    sqlite3* db = (sqlite3*)storage_;

    // 使用事务保证原子性，失败时重试（数据库可能被打卡/profile写入持锁）
    constexpr int MAX_RETRIES = 3;
    constexpr int RETRY_DELAY_MS = 50;
    bool beginOk = false;
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) == SQLITE_OK) {
            beginOk = true;
            break;
        }
        LOG_WARNING(TEXT("ProfileManager: AddDailyLike BEGIN IMMEDIATE failed (attempt %d/%d): %hs"),
            attempt + 1, MAX_RETRIES, sqlite3_errmsg(db));
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
    }
    if (!beginOk) {
        LOG_ERROR(TEXT("ProfileManager: AddDailyLike BEGIN IMMEDIATE failed after %d retries, like dropped for uid=%hs date=%d count=%d: %hs"),
            MAX_RETRIES, uid.c_str(), likeDate, likeCount, sqlite3_errmsg(db));
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    const char* updateSql = "UPDATE user_daily_likes SET total_likes = total_likes + ? WHERE uid = ? AND like_date = ?";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, likeCount);
        sqlite3_bind_text(stmt, 2, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, likeDate);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int changes = sqlite3_changes(db);
            sqlite3_finalize(stmt);

            if (changes > 0) {
                const char* selectSql = "SELECT total_likes FROM user_daily_likes WHERE uid = ? AND like_date = ?";
                if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 2, likeDate);
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        outTotalLikes = sqlite3_column_int(stmt, 0);
                    } else {
                        LOG_ERROR(TEXT("ProfileManager: AddDailyLike SELECT failed after UPDATE: %hs"), sqlite3_errmsg(db));
                        sqlite3_finalize(stmt);
                        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
                        return false;
                    }
                    sqlite3_finalize(stmt);
                    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
                        LOG_ERROR(TEXT("ProfileManager: AddDailyLike COMMIT failed: %hs"), sqlite3_errmsg(db));
                        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
                        return false;
                    }
                    return true;
                }
            } else {
                const char* insertSql = "INSERT INTO user_daily_likes (uid, like_date, total_likes) VALUES (?, ?, ?)";
                if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmt, 2, likeDate);
                    sqlite3_bind_int(stmt, 3, likeCount);
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        outTotalLikes = likeCount;
                        sqlite3_finalize(stmt);
                        if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
                            LOG_ERROR(TEXT("ProfileManager: AddDailyLike COMMIT failed: %hs"), sqlite3_errmsg(db));
                            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
                            return false;
                        }
                        return true;
                    }
                    sqlite3_finalize(stmt);
                }
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return false;
}

bool ProfileManager::LoadProfileFromDb(const std::string& uid, UserProfileData& outProfile) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    const char* sql = "SELECT uid, username, last_checkin_date, continuous_days, cumulative_days, last_danmu_timestamp, created_at, updated_at, keywords_json, danmu_history_json FROM user_profiles WHERE uid = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to prepare select statement: %hs"), sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outProfile.uid = (const char*)sqlite3_column_text(stmt, 0);
        const char* rawUsername = (const char*)sqlite3_column_text(stmt, 1);
        outProfile.username = rawUsername ? rawUsername : "";
        outProfile.lastCheckinDate = sqlite3_column_int(stmt, 2);
        outProfile.continuousDays = sqlite3_column_int(stmt, 3);
        outProfile.cumulativeDays = sqlite3_column_int(stmt, 4);
        outProfile.lastDanmuTimestamp = sqlite3_column_int64(stmt, 5);
        outProfile.createdAt = sqlite3_column_int64(stmt, 6);
        outProfile.updatedAt = sqlite3_column_int64(stmt, 7);

        const char* keywordsJson = (const char*)sqlite3_column_text(stmt, 8);
        if (keywordsJson) {
            outProfile.keywords = JsonToKeywords(keywordsJson);
        }

        const char* danmuHistoryJson = (const char*)sqlite3_column_text(stmt, 9);
        if (danmuHistoryJson) {
            outProfile.danmuHistory = JsonToDanmuHistory(danmuHistoryJson);
        }

        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

void ProfileManager::SaveProfileToDb(const UserProfileData& profile) {
    if (!storage_) return;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO user_profiles (uid, username, last_checkin_date, continuous_days, cumulative_days, last_danmu_timestamp, created_at, updated_at, keywords_json, danmu_history_json) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to prepare insert statement: %hs"), sqlite3_errmsg(db));
        return;
    }
    
    sqlite3_bind_text(stmt, 1, profile.uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, profile.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, profile.lastCheckinDate);
    sqlite3_bind_int(stmt, 4, profile.continuousDays);
    sqlite3_bind_int(stmt, 5, profile.cumulativeDays);
    sqlite3_bind_int64(stmt, 6, profile.lastDanmuTimestamp);
    sqlite3_bind_int64(stmt, 7, profile.createdAt);
    sqlite3_bind_int64(stmt, 8, GetCurrentTimestamp());
    sqlite3_bind_text(stmt, 9, KeywordsToJson(profile.keywords).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, DanmuHistoryToJson(profile.danmuHistory).c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR(TEXT("ProfileManager: Failed to execute insert: %hs"), sqlite3_errmsg(db));
    }
    
    sqlite3_finalize(stmt);
}

static std::string EscapeJsonString(const std::string& input) {
    std::ostringstream oss;
    for (char c : input) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (c >= 0 && c < 32) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string ProfileManager::KeywordsToJson(const std::vector<KeywordRecord>& keywords) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < keywords.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"word\":\"" << EscapeJsonString(keywords[i].word) << "\","
            << "\"freq\":" << keywords[i].frequency << ","
            << "\"ts\":" << keywords[i].lastSeenTimestamp << "}";
    }
    oss << "]";
    return oss.str();
}

std::vector<KeywordRecord> ProfileManager::JsonToKeywords(const std::string& json) const {
    std::vector<KeywordRecord> result;
    if (json.empty() || json == "[]") return result;

    try {
        auto j = nlohmann::json::parse(json);
        for (const auto& item : j) {
            KeywordRecord record;
            record.word = item.value("word", "");
            record.frequency = item.value("freq", 0);
            record.lastSeenTimestamp = item.value("ts", 0);
            result.push_back(record);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(TEXT("ProfileManager: Failed to parse keywords JSON: %hs"), e.what());
    }
    return result;
}

std::string ProfileManager::DanmuHistoryToJson(const std::vector<std::pair<int64_t, std::string>>& history) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "[" << history[i].first << ",\"" << EscapeJsonString(history[i].second) << "\"]";
    }
    oss << "]";
    return oss.str();
}

std::vector<std::pair<int64_t, std::string>> ProfileManager::JsonToDanmuHistory(const std::string& json) const {
    std::vector<std::pair<int64_t, std::string>> result;
    if (json.empty() || json == "[]") return result;

    try {
        auto j = nlohmann::json::parse(json);
        for (const auto& item : j) {
            if (item.is_array() && item.size() == 2) {
                result.emplace_back(item[0].get<int64_t>(), item[1].get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR(TEXT("ProfileManager: Failed to parse danmu history JSON: %hs"), e.what());
    }
    return result;
}

void ProfileManager::SaveProfile(const UserProfileData& profile) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);
    profiles_[profile.uid] = profile;
    EvictOldestProfileIfNeeded();
    SaveProfileToDb(profile);
    LOG_INFO(TEXT("ProfileManager: Saved profile for uid=%hs"), profile.uid.c_str());
}

bool ProfileManager::LoadProfile(const std::string& uid, UserProfileData& outProfile) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);

    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        outProfile = it->second;
        return true;
    }

    if (LoadProfileFromDb(uid, outProfile)) {
        profiles_[uid] = outProfile;
        EvictOldestProfileIfNeeded();
        return true;
    }

    return false;
}

void ProfileManager::DeleteProfile(const std::string& uid) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);
    profiles_.erase(uid);
    if (storage_) {
        sqlite3* db = (sqlite3*)storage_;
        const char* sql = "DELETE FROM user_profiles WHERE uid = ?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    LOG_INFO(TEXT("ProfileManager: Deleted profile for uid=%hs"), uid.c_str());
}

void ProfileManager::RecordCheckin(const std::string& uid, const std::string& username, int32_t checkinDate) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);

    int32_t continuousDays = CalculateContinuousDays(uid, checkinDate);

    UserProfileData profile;
    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        profile = it->second;
    }
    else {
        profile.uid = uid;
        profile.username = username;
        profile.createdAt = GetCurrentTimestamp();
        profile.updatedAt = GetCurrentTimestamp();
    }
    
    // 累计天数：非重复打卡时递增
    profile.cumulativeDays = CalculateCumulativeDays(uid, checkinDate);
    
    profile.lastCheckinDate = checkinDate;
    profile.continuousDays = continuousDays;

    bool dbSuccess = true;
    if (storage_) {
        sqlite3* db = (sqlite3*)storage_;

        // 开始事务
        if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
            LOG_ERROR(TEXT("ProfileManager: RecordCheckin BEGIN failed: %hs"), sqlite3_errmsg(db));
            return;
        }

        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?) ON CONFLICT(uid, checkin_date) DO UPDATE SET created_at = excluded.created_at, username = excluded.username WHERE checkin_records.created_at != excluded.created_at";

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, checkinDate);
            sqlite3_bind_int64(stmt, 3, GetCurrentTimestamp());
            sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_TRANSIENT);
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                LOG_ERROR(TEXT("ProfileManager: Failed to insert checkin record: %hs"), sqlite3_errmsg(db));
                dbSuccess = false;
            }
            sqlite3_finalize(stmt);
        } else {
            LOG_ERROR(TEXT("ProfileManager: Failed to prepare checkin insert: %hs"), sqlite3_errmsg(db));
            dbSuccess = false;
        }

        if (dbSuccess) {
            SaveProfileToDb(profile);
            profiles_[uid] = profile;
            EvictOldestProfileIfNeeded();
            if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
                LOG_ERROR(TEXT("ProfileManager: RecordCheckin COMMIT failed: %hs"), sqlite3_errmsg(db));
                dbSuccess = false;
            }
        }

        if (!dbSuccess) {
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    LOG_INFO(TEXT("ProfileManager: Recorded checkin for uid=%hs, days=%d"), uid.c_str(), continuousDays);
}

void ProfileManager::RecordCheckinAsync(const std::string& uid, const std::string& username, int32_t checkinDate, int32_t continuousDays, int32_t cumulativeDays) {
    auto* self = this;
    std::thread([self, uid, username, checkinDate, continuousDays, cumulativeDays]() {
        self->RecordCheckinSync(uid, username, checkinDate, continuousDays, cumulativeDays);
    }).detach();
}

bool ProfileManager::RecordCheckinSync(const std::string& uid, const std::string& username, int32_t checkinDate, int32_t continuousDays, int32_t cumulativeDays) {
    UserProfileData profile;
    int64_t timestamp = GetCurrentTimestamp();
    {
        std::lock_guard<std::recursive_mutex> lock(profilesLock_);
        auto it = profiles_.find(uid);
        if (it != profiles_.end()) {
            profile = it->second;
        } else {
            profile.uid = uid;
            profile.username = username;
            profile.createdAt = timestamp;
            profile.updatedAt = timestamp;
        }
    }
    profile.lastCheckinDate = checkinDate;
    profile.continuousDays = continuousDays;
    profile.cumulativeDays = cumulativeDays;

    bool dbSuccess = true;
    if (storage_) {
        sqlite3* db = (sqlite3*)storage_;

        if (!InsertCheckinRecordWithRetry(db, uid, checkinDate, timestamp, username)) {
            LOG_ERROR(TEXT("ProfileManager: Checkin record INSERT failed for uid=%hs, username=%hs, date=%d, days=%d"),
                uid.c_str(), username.c_str(), checkinDate, continuousDays);
            dbSuccess = false;
        }

        if (dbSuccess) {
            for (int32_t retry = 0; retry < CHECKIN_RETRY_MAX; ++retry) {
                SaveProfileToDb(profile);
                {
                    std::lock_guard<std::recursive_mutex> lock(profilesLock_);
                    profiles_[uid] = profile;
                    EvictOldestProfileIfNeeded();
                }
                {
                    std::lock_guard<std::recursive_mutex> lock(profilesLock_);
                    auto it = profiles_.find(uid);
                    if (it != profiles_.end() && it->second.lastCheckinDate == checkinDate) {
                        break;
                    }
                }
                LOG_WARNING(TEXT("ProfileManager: SaveProfileToDb verification failed (attempt %d/%d), retrying..."),
                    retry + 1, CHECKIN_RETRY_MAX);
                if (retry < CHECKIN_RETRY_MAX - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(CHECKIN_RETRY_DELAY_MS));
                }
            }
        }
    } else {
        LOG_ERROR(TEXT("ProfileManager: storage_ is null, cannot save checkin record for uid=%hs"), uid.c_str());
        dbSuccess = false;
    }

    if (dbSuccess) {
        LOG_INFO(TEXT("ProfileManager: Recorded checkin sync for uid=%hs, username=%hs, days=%d, date=%d"),
            uid.c_str(), username.c_str(), continuousDays, checkinDate);
    } else {
        LOG_ERROR(TEXT("ProfileManager: Failed to record checkin for uid=%hs, username=%hs, days=%d, date=%d"),
            uid.c_str(), username.c_str(), continuousDays, checkinDate);
    }
    return dbSuccess;
}


bool ProfileManager::GetLastCheckinRecordFromDb(const std::string& uid, int32_t& outLastDate, int32_t& outContinuousDays) {
    {
        std::lock_guard<std::recursive_mutex> lock(profilesLock_);
        auto it = profiles_.find(uid);
        if (it != profiles_.end()) {
            outLastDate = it->second.lastCheckinDate;
            outContinuousDays = it->second.continuousDays;
            return outLastDate != 0;
        }
    }

    if (!storage_) {
        return false;
    }

    sqlite3* db = (sqlite3*)storage_;
    const char* sql = "SELECT last_checkin_date, continuous_days FROM user_profiles WHERE uid = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outLastDate = sqlite3_column_int(stmt, 0);
        outContinuousDays = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);

        {
            std::lock_guard<std::recursive_mutex> lock(profilesLock_);
            auto it = profiles_.find(uid);
            if (it != profiles_.end()) {
                it->second.lastCheckinDate = outLastDate;
                it->second.continuousDays = outContinuousDays;
            } else {
                UserProfileData newProfile;
                newProfile.uid = uid;
                newProfile.lastCheckinDate = outLastDate;
                newProfile.continuousDays = outContinuousDays;
                newProfile.updatedAt = GetCurrentTimestamp();
                profiles_[uid] = newProfile;
                EvictOldestProfileIfNeeded();
            }
        }

        return outLastDate != 0;
    }

    sqlite3_finalize(stmt);
    return false;
}

int32_t ProfileManager::CalculateContinuousDays(const std::string& uid, int32_t checkinDate) {
    int32_t lastDate = 0;
    int32_t lastContinuousDays = 0;

    if (!GetLastCheckinRecordFromDb(uid, lastDate, lastContinuousDays)) {
        return 1;
    }

    if (lastDate == 0) {
        return 1;
    }

    int32_t year = checkinDate / 10000;
    int32_t month = (checkinDate % 10000) / 100;
    int32_t day = checkinDate % 100;

    int32_t lastYear = lastDate / 10000;
    int32_t lastMonth = (lastDate % 10000) / 100;
    int32_t lastDay = lastDate % 100;

    if (year == lastYear && month == lastMonth) {
        if (day == lastDay + 1) {
            return lastContinuousDays + 1;
        }
        else if (day == lastDay) {
            return lastContinuousDays;
        }
    }
    else if (year == lastYear && month == lastMonth + 1) {
        int32_t lastMonthDays = DateUtils::GetDaysInMonth(lastYear, lastMonth);
        if (day == 1 && lastDay == lastMonthDays) {
            return lastContinuousDays + 1;
        }
    }
    else if (year == lastYear + 1) {
        if (month == 1 && lastMonth == 12) {
            if (day == 1 && lastDay == 31) {
                return lastContinuousDays + 1;
            }
        }
    }

    return 1;
}

int32_t ProfileManager::CalculateCumulativeDays(const std::string& uid, int32_t checkinDate) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);

    UserProfileData profile;
    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        profile = it->second;
    } else {
        return 1;
    }

    // 累计天数：非重复打卡时递增
    if (profile.lastCheckinDate != checkinDate) {
        if (profile.cumulativeDays > 0) {
            return profile.cumulativeDays + 1;
        } else {
            return 1;
        }
    }
    return profile.cumulativeDays > 0 ? profile.cumulativeDays : 1;
}

int32_t ProfileManager::CalculateContinuousDaysFromRecords(const std::string& uid) {
    if (!storage_) return 1;

    sqlite3* db = (sqlite3*)storage_;
    sqlite3_stmt* stmt = nullptr;

    // 查询用户的所有打卡记录，按日期降序排列
    const char* sql = "SELECT checkin_date FROM checkin_records WHERE uid = ? ORDER BY checkin_date DESC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 1;
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<int32_t> dates;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dates.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    if (dates.empty()) {
        return 1;
    }

    // 从最新日期开始往前遍历，计算连续天数
    int32_t continuousDays = 1;
    for (size_t i = 1; i < dates.size(); ++i) {
        if (DateUtils::IsNextCalendarDay(dates[i], dates[i - 1])) {
            continuousDays++;
        } else {
            break;
        }
    }

    return continuousDays;
}

void ProfileManager::AddKeyword(const std::string& uid, const std::string& keyword) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);

    auto it = profiles_.find(uid);
    if (it == profiles_.end()) return;

    UserProfileData& profile = it->second;
    bool found = false;
    for (auto& kw : profile.keywords) {
        if (kw.word == keyword) {
            kw.frequency++;
            kw.lastSeenTimestamp = GetCurrentTimestamp();
            found = true;
            break;
        }
    }
    if (!found) {
        if (profile.keywords.size() >= 50) {
            auto minIt = std::min_element(profile.keywords.begin(), profile.keywords.end(),
                [](const KeywordRecord& a, const KeywordRecord& b) { return a.frequency < b.frequency; });
            if (minIt != profile.keywords.end()) {
                profile.keywords.erase(minIt);
            }
        }
        profile.keywords.push_back({0, keyword, 1, GetCurrentTimestamp()});
    }
    SaveProfileToDb(profile);
}

void ProfileManager::AddDanmuHistory(const std::string& uid, int64_t timestamp, const std::string& content) {
    std::lock_guard<std::recursive_mutex> lock(profilesLock_);

    auto it = profiles_.find(uid);
    if (it == profiles_.end()) return;

    UserProfileData& profile = it->second;
    profile.danmuHistory.push_back({timestamp, content});
    if (profile.danmuHistory.size() > 100) {
        profile.danmuHistory.erase(profile.danmuHistory.begin());
    }
    profile.lastDanmuTimestamp = timestamp;
    SaveProfileToDb(profile);
}

std::string ProfileManager::SerializeToJson(const UserProfileData& profile) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"uid\":\"" << profile.uid << "\",";
    oss << "\"username\":\"" << EscapeJsonString(profile.username) << "\",";
    oss << "\"lastCheckinDate\":" << profile.lastCheckinDate << ",";
    oss << "\"continuousDays\":" << profile.continuousDays << ",";
    oss << "\"lastDanmuTimestamp\":" << profile.lastDanmuTimestamp << ",";
    oss << "\"createdAt\":" << profile.createdAt << ",";
    oss << "\"keywords\":" << KeywordsToJson(profile.keywords) << ",";
    oss << "\"danmuHistory\":" << DanmuHistoryToJson(profile.danmuHistory);
    oss << "}";
    return oss.str();
}

bool ProfileManager::DeserializeFromJson(const std::string& json, UserProfileData& outProfile) {
    if (json.empty() || json == "{}") return false;

    try {
        auto j = nlohmann::json::parse(json);
        outProfile.uid = j.value("uid", "");
        outProfile.username = j.value("username", "");
        outProfile.lastCheckinDate = j.value("lastCheckinDate", 0);
        outProfile.continuousDays = j.value("continuousDays", 0);
        outProfile.lastDanmuTimestamp = j.value("lastDanmuTimestamp", 0);
        outProfile.createdAt = j.value("createdAt", 0);

        if (j.contains("keywords")) {
            outProfile.keywords = JsonToKeywords(j["keywords"].dump());
        }
        if (j.contains("danmuHistory")) {
            outProfile.danmuHistory = JsonToDanmuHistory(j["danmuHistory"].dump());
        }

        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("ProfileManager: Failed to parse JSON: %hs"), e.what());
        return false;
    }
}

void ProfileManager::EvictOldestProfileIfNeeded() {
    if (profiles_.size() <= 1000) return;

    auto oldest = profiles_.end();
    int64_t oldestTime = (std::numeric_limits<int64_t>::max)();
    for (auto it = profiles_.begin(); it != profiles_.end(); ++it) {
        if (it->second.updatedAt == 0) continue; // skip profiles with uninitialized updatedAt
        if (it->second.updatedAt < oldestTime) {
            oldestTime = it->second.updatedAt;
            oldest = it;
        }
    }
    if (oldest != profiles_.end()) {
        profiles_.erase(oldest);
    }
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

    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
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
        if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
            LOG_ERROR(TEXT("ProfileManager: DeductRetroactiveCard COMMIT failed: %hs"), sqlite3_errmsg(db));
            return false;
        }
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
}

bool ProfileManager::AddRetroactiveCard(const std::string& uid, int32_t count) {
    if (!storage_) return false;
    if (count <= 0) return false;

    sqlite3* db = (sqlite3*)storage_;

    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* selectSql = "SELECT card_count FROM retroactive_cards WHERE uid = ?";

    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = true;
    }
    sqlite3_finalize(stmt);

    bool success = false;
    if (exists) {
        const char* updateSql = "UPDATE retroactive_cards SET card_count = card_count + ?, total_earned = total_earned + ? WHERE uid = ?";
        if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, count);
            sqlite3_bind_int(stmt, 2, count);
            sqlite3_bind_text(stmt, 3, uid.c_str(), -1, SQLITE_TRANSIENT);
            success = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
        }
    } else {
        const char* insertSql = "INSERT INTO retroactive_cards (uid, card_count, total_earned) VALUES (?, ?, ?)";
        if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, count);
            sqlite3_bind_int(stmt, 3, count);
            success = sqlite3_step(stmt) == SQLITE_DONE;
            sqlite3_finalize(stmt);
        }
    }

    if (success) {
        if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
            LOG_ERROR(TEXT("ProfileManager: AddRetroactiveCard COMMIT failed: %hs"), sqlite3_errmsg(db));
            return false;
        }
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
}

bool ProfileManager::IssueStreakReward(const std::string& uid, int32_t date) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    // 原子性更新 streak_reward_issued 和补签卡数量，添加幂等保护
    const char* updateSql = 
        "UPDATE user_like_streaks SET streak_reward_issued = ? WHERE uid = ? AND streak_reward_issued != ?";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_text(stmt, 2, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, date);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(stmt);

    // 检查是否真正更新了记录（幂等性）
    if (sqlite3_changes(db) == 0) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    // 增加补签卡
    const char* cardSql = 
        "UPDATE retroactive_cards SET card_count = card_count + 1, total_earned = total_earned + 1, last_earned_date = ? WHERE uid = ?";
    if (sqlite3_prepare_v2(db, cardSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_text(stmt, 2, uid.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(stmt);

    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: IssueStreakReward COMMIT failed: %hs"), sqlite3_errmsg(db));
        return false;
    }
    return true;
}

bool ProfileManager::IssueMonthlyFirstReward(const std::string& uid, int32_t date) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;

    // 原子性更新 monthly_first_claimed 和补签卡数量，添加幂等保护
    // 只有当 monthly_first_claimed 不是当前月时才更新
    int32_t currentMonth = date / 100;
    const char* updateSql = 
        "UPDATE retroactive_cards SET card_count = card_count + 1, total_earned = total_earned + 1, monthly_first_claimed = ?, last_earned_date = ? WHERE uid = ? AND (monthly_first_claimed = 0 OR monthly_first_claimed / 100 != ?)";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_int(stmt, 2, date);
    sqlite3_bind_text(stmt, 3, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, currentMonth);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(stmt);

    // 检查是否真正更新了记录（幂等性）
    if (sqlite3_changes(db) == 0) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: IssueMonthlyFirstReward COMMIT failed: %hs"), sqlite3_errmsg(db));
        return false;
    }
    return true;
}

bool ProfileManager::ExecuteRetroactiveCheckin(const std::string& uid, const std::string& username, int32_t targetDate, int32_t& outNewCardCount) {
    outNewCardCount = 0;
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;

    // 开始事务
    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    bool success = false;

    // 1. 检查并扣减补签卡
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
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(stmt);
    outNewCardCount = cardCount - 1;

    // 2. 插入补签记录
    const char* insertSql = "INSERT OR REPLACE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        outNewCardCount = 0;
        return false;
    }
    sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, targetDate);
    sqlite3_bind_int64(stmt, 3, GetCurrentTimestamp());
    sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        outNewCardCount = 0;
        return false;
    }
    sqlite3_finalize(stmt);

    // 3. 更新用户 profile（补签后从 checkin_records 重新计算连续天数）
    UserProfileData profile;
    if (LoadProfileFromDb(uid, profile)) {
        int32_t newContinuousDays = CalculateContinuousDaysFromRecords(uid);
        profile.continuousDays = newContinuousDays;
        // 补签时累计天数 +1
        if (profile.cumulativeDays > 0) {
            profile.cumulativeDays += 1;
        } else {
            profile.cumulativeDays = 1;
        }
        if (targetDate > profile.lastCheckinDate) {
            profile.lastCheckinDate = targetDate;
        }
        profile.updatedAt = GetCurrentTimestamp();
        SaveProfileToDb(profile);
    }

    // 提交事务
    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: ExecuteRetroactiveCheckin COMMIT failed: %hs"), sqlite3_errmsg(db));
        return false;
    }
    return true;
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

    const char* sql = "SELECT MAX(checkin_date) FROM checkin_records WHERE uid = ?";

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
        return 0;
    }

    if (lastCheckinDate >= currentDate) {
        return 0;
    }

    int32_t year = currentDate / 10000;
    int32_t month = (currentDate % 10000) / 100;
    int32_t day = currentDate % 100;

    int32_t lastYear = lastCheckinDate / 10000;
    int32_t lastMonth = (lastCheckinDate % 10000) / 100;
    int32_t lastDay = lastCheckinDate % 100;

    if (year == lastYear && month == lastMonth && day == lastDay + 1) {
        return 0;
    }
    int32_t lastMonthDays = DateUtils::GetDaysInMonth(lastYear, lastMonth);
    if (year == lastYear && month == lastMonth + 1 && day == 1 && lastDay == lastMonthDays) {
        return 0;
    }
    if (year == lastYear + 1 && month == 1 && lastMonth == 12 && day == 1 && lastDay == 31) {
        return 0;
    }

    int32_t missingDate = currentDate;
    if (day > 1) {
        missingDate = currentDate - 1;
    } else {
        int32_t prevMonth = month - 1;
        int32_t prevYear = year;
        if (prevMonth == 0) {
            prevMonth = 12;
            prevYear--;
        }
        int32_t prevMonthDays = DateUtils::GetDaysInMonth(prevYear, prevMonth);
        missingDate = prevYear * 10000 + prevMonth * 100 + prevMonthDays;
    }

    const char* checkSql = "SELECT 1 FROM checkin_records WHERE uid = ? AND checkin_date = ?";
    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, uid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, missingDate);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return 0;
        }
        sqlite3_finalize(stmt);
    }

    return missingDate;
}
