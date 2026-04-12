#include "framework.h"
#include "ProfileManager.h"
#include "WriteLog.h"
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
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
        "uid INTEGER PRIMARY KEY,"
        "username TEXT NOT NULL,"
        "last_checkin_date INTEGER DEFAULT 0,"
        "continuous_days INTEGER DEFAULT 0,"
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
        "uid INTEGER NOT NULL,"
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

    storage_ = (void*)db;

    LOG_INFO(TEXT("ProfileManager::Init success"));
    return true;
}

bool ProfileManager::LoadProfileFromDb(uint64_t uid, UserProfileData& outProfile) {
    if (!storage_) return false;

    sqlite3* db = (sqlite3*)storage_;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT uid, username, last_checkin_date, continuous_days, last_danmu_timestamp, created_at, updated_at, keywords_json, danmu_history_json FROM user_profiles WHERE uid = %llu",
        (unsigned long long)uid);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to prepare select statement: %hs"), sqlite3_errmsg(db));
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outProfile.uid = sqlite3_column_int64(stmt, 0);
        outProfile.username = (const char*)sqlite3_column_text(stmt, 1);
        outProfile.lastCheckinDate = sqlite3_column_int(stmt, 2);
        outProfile.continuousDays = sqlite3_column_int(stmt, 3);
        outProfile.lastDanmuTimestamp = sqlite3_column_int64(stmt, 4);
        outProfile.createdAt = sqlite3_column_int64(stmt, 5);
        outProfile.updatedAt = sqlite3_column_int64(stmt, 6);

        const char* keywordsJson = (const char*)sqlite3_column_text(stmt, 7);
        if (keywordsJson) {
            outProfile.keywords = JsonToKeywords(keywordsJson);
        }

        const char* danmuHistoryJson = (const char*)sqlite3_column_text(stmt, 8);
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
    const char* sql = "INSERT OR REPLACE INTO user_profiles (uid, username, last_checkin_date, continuous_days, last_danmu_timestamp, created_at, updated_at, keywords_json, danmu_history_json) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR(TEXT("ProfileManager: Failed to prepare insert statement: %hs"), sqlite3_errmsg(db));
        return;
    }
    
    sqlite3_bind_int64(stmt, 1, profile.uid);
    sqlite3_bind_text(stmt, 2, profile.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, profile.lastCheckinDate);
    sqlite3_bind_int(stmt, 4, profile.continuousDays);
    sqlite3_bind_int64(stmt, 5, profile.lastDanmuTimestamp);
    sqlite3_bind_int64(stmt, 6, profile.createdAt);
    sqlite3_bind_int64(stmt, 7, GetCurrentTimestamp());
    sqlite3_bind_text(stmt, 8, KeywordsToJson(profile.keywords).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, DanmuHistoryToJson(profile.danmuHistory).c_str(), -1, SQLITE_TRANSIENT);
    
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
    std::lock_guard<std::mutex> lock(profilesLock_);
    profiles_[profile.uid] = profile;
    SaveProfileToDb(profile);
    LOG_INFO(TEXT("ProfileManager: Saved profile for uid=%llu"), profile.uid);
}

bool ProfileManager::LoadProfile(uint64_t uid, UserProfileData& outProfile) {
    std::lock_guard<std::mutex> lock(profilesLock_);

    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        outProfile = it->second;
        return true;
    }

    if (LoadProfileFromDb(uid, outProfile)) {
        profiles_[uid] = outProfile;
        return true;
    }

    return false;
}

void ProfileManager::DeleteProfile(uint64_t uid) {
    std::lock_guard<std::mutex> lock(profilesLock_);
    profiles_.erase(uid);
    if (storage_) {
        sqlite3* db = (sqlite3*)storage_;
        char sql[256];
        snprintf(sql, sizeof(sql), "DELETE FROM user_profiles WHERE uid = %llu", uid);
        char* errMsg = nullptr;
        sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
        if (errMsg) {
            sqlite3_free(errMsg);
        }
    }
    LOG_INFO(TEXT("ProfileManager: Deleted profile for uid=%llu"), uid);
}

void ProfileManager::RecordCheckin(uint64_t uid, const std::string& username, int32_t checkinDate) {
    std::lock_guard<std::mutex> lock(profilesLock_);

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
    }
    profile.lastCheckinDate = checkinDate;
    profile.continuousDays = continuousDays;

    bool dbSuccess = true;
    if (storage_) {
        sqlite3* db = (sqlite3*)storage_;
        
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, uid);
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
        }
    }

    LOG_INFO(TEXT("ProfileManager: Recorded checkin for uid=%llu, days=%d"), uid, continuousDays);
}

void ProfileManager::RecordCheckinAsync(uint64_t uid, const std::string& username, int32_t checkinDate, int32_t continuousDays) {
    auto* self = this;
    std::thread([self, uid, username, checkinDate, continuousDays]() {
        UserProfileData profile;
        int64_t timestamp = GetCurrentTimestamp();
        {
            std::lock_guard<std::mutex> lock(self->profilesLock_);
            auto it = self->profiles_.find(uid);
            if (it != self->profiles_.end()) {
                profile = std::move(it->second);
            } else {
                profile.uid = uid;
                profile.username = username;
                profile.createdAt = timestamp;
            }
        }
        profile.lastCheckinDate = checkinDate;
        profile.continuousDays = continuousDays;

        bool dbSuccess = true;
        if (self->storage_) {
            sqlite3* db = (sqlite3*)self->storage_;
            sqlite3_stmt* stmt = nullptr;
            const char* sql = "INSERT OR IGNORE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)";
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(stmt, 1, (sqlite3_int64)uid);
                sqlite3_bind_int(stmt, 2, checkinDate);
                sqlite3_bind_int64(stmt, 3, timestamp);
                sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_TRANSIENT);
                
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    LOG_ERROR(TEXT("ProfileManager: Failed to insert checkin record async: %hs"), sqlite3_errmsg(db));
                    dbSuccess = false;
                }
                sqlite3_finalize(stmt);
            } else {
                LOG_ERROR(TEXT("ProfileManager: Failed to prepare checkin insert async: %hs"), sqlite3_errmsg(db));
                dbSuccess = false;
            }

            if (dbSuccess) {
                self->SaveProfileToDb(profile);
                std::lock_guard<std::mutex> lock(self->profilesLock_);
                self->profiles_[uid] = profile;
            }
        }

        LOG_INFO(TEXT("ProfileManager: Recorded checkin async for uid=%llu, days=%d"), uid, continuousDays);
    }).detach();
}

namespace {
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

bool ProfileManager::GetLastCheckinRecordFromDb(uint64_t uid, int32_t& outLastDate, int32_t& outContinuousDays) {
    if (!storage_) {
        auto it = profiles_.find(uid);
        if (it != profiles_.end()) {
            outLastDate = it->second.lastCheckinDate;
            outContinuousDays = it->second.continuousDays;
            return outLastDate != 0;
        }
        return false;
    }

    sqlite3* db = (sqlite3*)storage_;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT last_checkin_date, continuous_days FROM user_profiles WHERE uid = %llu",
        (unsigned long long)uid);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outLastDate = sqlite3_column_int(stmt, 0);
        outContinuousDays = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return outLastDate != 0;
    }

    sqlite3_finalize(stmt);
    return false;
}

int32_t ProfileManager::CalculateContinuousDays(uint64_t uid, int32_t checkinDate) {
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
        int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
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
        else if (month == 2 && lastMonth == 1) {
            int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
            if (day == 1 && lastDay == lastMonthDays) {
                return lastContinuousDays + 1;
            }
        }
    }
    else if (year == lastYear + 2 && month == 1 && lastMonth == 12) {
        if (day == 1 && lastDay == 31) {
            return lastContinuousDays + 1;
        }
    }

    return 1;
}

void ProfileManager::AddKeyword(uint64_t uid, const std::string& keyword) {
    std::lock_guard<std::mutex> lock(profilesLock_);

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

void ProfileManager::AddDanmuHistory(uint64_t uid, int64_t timestamp, const std::string& content) {
    std::lock_guard<std::mutex> lock(profilesLock_);

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
    oss << "\"uid\":" << profile.uid << ",";
    oss << "\"username\":\"" << profile.username << "\",";
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
        outProfile.uid = j.value("uid", 0);
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
