# Captain CheckIn AI Reply - Design

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    C# 配置层 (MonsterOrderWildsGUI)          │
│  - 舰长打卡AI开关 (CheckBox)                               │
│  - 打卡触发词编辑框                                        │
│  - AI API Key 配置（加密存储）                              │
└─────────────────────┬───────────────────────────────────────┘
                      │ ConfigBridge
┌─────────────────────▼───────────────────────────────────────┐
│                    C++ 核心模块 (CaptainCheckInModule)      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │ 开关判断   │→ │ 打卡检测器  │→ │ AI回复生成器       │ │
│  │ (总开关)   │  │ (触发词匹配)│  │ (MiniMax API)      │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                         ↓                                    │
│                  ┌─────────────┐                            │
│                  │ 用户画像库   │                            │
│                  │ (词袋+关键词)│                            │
│                  └─────────────┘                            │
└─────────────────────────────────────────────────────────────┘
```

**功能总开关**：
- `ENABLE_CAPTAIN_CHECKIN_AI` 控制整个功能的开启/关闭
- 关闭时：不进行学习、统计、AI回复等任何操作

## Module Structure

### cppjieba 分词集成

#### 目录结构
```
external/
├── cppjieba/                      # cppjieba 源码
│   ├── include/
│   │   └── cppjieba/              # 16个头文件
│   │       ├── Jieba.hpp          # 主入口
│   │       ├── DictTrie.hpp
│   │       ├── MPSegment.hpp
│   │       ├── HMMSegment.hpp
│   │       ├── MixSegment.hpp
│   │       └── ... (其他)
│   ├── deps/
│   │   └── limonp/               # limonp 依赖库
│   │       └── include/
│   │           └── limonp/
│   │               ├── StringUtil.hpp
│   │               └── ... (其他)
MonsterOrderWilds/
└── dict/                          # 词典文件
    ├── jieba.dict.utf8            # 最大概率分词词典 (~4MB)
    ├── hmm_model.utf8             # HMM模型 (~200KB)
    └── stop_words.utf8            # 停用词词典
```

**注意**：cppjieba 源码在 `external/cppjieba/`，词典文件在 `MonsterOrderWilds/dict/`。CaptainCheckInModule 使用 `GetModuleDictPath()` 动态获取 exe 所在目录的 `dict/` 子目录。

#### cppjieba 初始化
```cpp
#include "cppjieba/Jieba.hpp"

class CaptainCheckInModule {
    // cppjieba 分词器（使用 MixSegment: MP+HMM 混合）
    cppjieba::Jieba jieba_;
    
    bool Init() {
        std::string dictPath = GetModuleDictPath();
        std::string jiebaDict = dictPath + "/jieba.dict.utf8";
        std::string hmmModel = dictPath + "/hmm_model.utf8";
        std::string userDict = "";  // 暂不启用自定义词典
        std::string stopWords = dictPath + "/stop_words.utf8";
        
        jieba_ = cppjieba::Jieba(jiebaDict, hmmModel, userDict, stopWords);
        return true;
    }
};
```

### Provider 抽象层

#### ITTSProvider - TTS 专用接口（必须实现）

**重要**：Task 2.6 必须实现完整的 ITTSProvider 接口和工厂类。

```cpp
class ITTSProvider {
public:
    virtual ~ITTSProvider() = default;
    virtual std::string GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual void RequestTTS(const TTSRequest& request, TTSCallback callback) = 0;
};

// TTS Provider 工厂（从完整 AI_PROVIDER JSON 中解析 tts_provider 和 tts_api_key）
class TTSProviderFactory {
public:
    static std::unique_ptr<ITTSProvider> Create(const std::string& credentialJson);
};

#### SapiTTSProvider - Windows SAPI 降级 Provider

当 `AI_PROVIDER` 中的 TTS Provider 不可用或 API Key 为空时，降级到 Windows SAPI。

```cpp
class SapiTTSProvider : public ITTSProvider {
public:
    SapiTTSProvider() = default;
    
    std::string GetProviderName() const override { return "sapi"; }
    
    bool IsAvailable() const override { return true; }
    
    std::string GetLastError() const override { return lastError_; }
    
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override {
        std::wstring wtext = StringUtil::Utf8ToWide(request.text);
        
        if (TTSManager::Inst()->SpeakWithSapi(wtext.c_str())) {
            TTSResponse response;
            response.audioData = {};  // SAPI 直接播放，无需返回音频数据
            callback(response);
        } else {
            TTSResponse response;
            response.errorMsg = "SAPI speak failed";
            callback(response);
        }
    }

private:
    std::string lastError_;
};
```

**降级策略**：
1. 优先使用 `AI_PROVIDER` 中指定的 TTS Provider（xiaomi 或 minimax）
2. 如果 Provider 不可用或 API Key 为空，降级到 Windows SAPI
3. Windows SAPI 通过 `TTSManager::SpeakWithSapi()` 实现

#### IAIChatProvider - 文本对话专用接口
```cpp
class IAIChatProvider {
public:
    virtual ~IAIChatProvider() = default;
    virtual std::string GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual bool CallAPI(const std::string& prompt, std::string& outResponse) = 0;
};

// AI Chat Provider 工厂（从完整 AI_PROVIDER JSON 中解析 chat_provider 和 chat_api_key）
class AIChatProviderFactory {
public:
    static std::unique_ptr<IAIChatProvider> Create(const std::string& credentialJson);
};
```

### ProfileManager 用户画像持久化管理

使用 **SQLite C API** 管理 SQLite 数据库，实现用户画像的持久化存储。

#### SQLite C API 简介

| 特性 | 说明 |
|------|------|
| 库类型 | 原生 C API |
| 头文件 | `sqlite3.h` |
| 依赖 | libsqlite3.lib / sqlite3.dll |
| 许可证 | 公共领域 |
| 主要功能 | CRUD、事务、预处理语句 |

#### 数据库结构

```sql
-- 用户画像表
CREATE TABLE user_profiles (
    uid INTEGER PRIMARY KEY,              -- 用户UID，主键
    username TEXT NOT NULL,               -- 用户名
    keywords TEXT,                        -- JSON: [{"word":"太刀","freq":5,"ts":123456},...]
    danmu_history TEXT,                   -- JSON: [{"ts":123456,"content":"内容"},...]
    last_checkin_date INTEGER DEFAULT 0,  -- 上次打卡日期 YYYYMMDD
    continuous_days INTEGER DEFAULT 0,     -- 连续打卡天数
    last_danmu_timestamp INTEGER DEFAULT 0,-- 上次弹幕时间戳
    created_at INTEGER DEFAULT 0,         -- 创建时间戳
    updated_at INTEGER DEFAULT 0          -- 更新时间戳
);

-- 每日打卡记录表（用于计算连续天数）
CREATE TABLE checkin_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uid INTEGER NOT NULL,
    checkin_date INTEGER NOT NULL,         -- YYYYMMDD
    created_at INTEGER NOT NULL,
    UNIQUE(uid, checkin_date)             -- 唯一约束：每人每天一条
);

-- 索引
CREATE INDEX idx_checkin_uid_date ON checkin_records(uid, checkin_date);
```

#### SQLite C API 数据结构

```cpp
// ProfileManager.h
#include "sqlite3.h"

struct UserProfileRecord {
    uint64_t uid = 0;                    // 主键
    std::string username;
    std::string keywordsJson;            // JSON string for keywords
    std::string danmuHistoryJson;        // JSON string for danmu history
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

struct CheckinRecordEntry {
    int64_t id = 0;
    uint64_t uid = 0;
    int32_t checkinDate = 0;
    int64_t createdAt = 0;
};
```

#### ProfileManager 接口

```cpp
class ProfileManager {
public:
    static ProfileManager& Instance();
    
    bool Init();  // 无参数，内部自动获取 dbPath
    void Destroy();
    
    // 用户画像 CRUD
    bool LoadProfile(uint64_t uid, UserProfileData& outProfile);
    void SaveProfile(const UserProfileData& profile);
    void DeleteProfile(uint64_t uid);
    
    // 打卡记录
    void RecordCheckin(uint64_t uid, const std::string& username, int32_t checkinDate);
    int32_t CalculateContinuousDays(uint64_t uid, int32_t checkinDate);
    
    // JSON 序列化/反序列化
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
    void* storage_ = nullptr;  // sqlite3* 指针
    std::map<uint64_t, UserProfileData> profiles_;  // 内存缓存
    std::mutex profilesLock_;
};
```

**数据库路径**：`GetModuleFileNameA()` 获取 exe 路径后拼接 `MonsterOrderWilds_configs\captain_profiles.db`

#### JSON 序列化实现

```cpp
std::string ProfileManager::DanmuHistoryToJson(
    const std::vector<std::pair<int64_t, std::string>>& history) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "[" << history[i].first << ",\"" << history[i].second << "\"]";
    }
    oss << "]";
    return oss.str();
}

std::vector<std::pair<int64_t, std::string>> ProfileManager::JsonToDanmuHistory(
    const std::string& json) const {
    std::vector<std::pair<int64_t, std::string>> result;
    if (json.empty() || json == "[]") return result;
    
    try {
        auto j = nlohmann::json::parse(json);
        for (const auto& item : j) {
            if (item.is_array() && item.size() == 2) {
                result.emplace_back(item[0].get<int64_t>(), item[1].get<std::string>());
            }
        }
    } catch (const std::exception&) {}
    return result;
}
```

#### ProfileManager 实现要点（SQLite C API）

```cpp
bool ProfileManager::Init() {
    if (storage_) return true;
    
    dbPath_ = GetDbPath();
    
    sqlite3* db = nullptr;
    int result = sqlite3_open(dbPath_.c_str(), &db);
    if (result != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    
    // 创建 user_profiles 表
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
    
    sqlite3_exec(db, createUserProfilesSql, nullptr, nullptr, nullptr);
    
    // 创建 checkin_records 表
    const char* createCheckinRecordsSql = 
        "CREATE TABLE IF NOT EXISTS checkin_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "uid INTEGER NOT NULL,"
        "checkin_date INTEGER NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "username TEXT,"
        "UNIQUE(uid, checkin_date)"
        ")";
    
    sqlite3_exec(db, createCheckinRecordsSql, nullptr, nullptr, nullptr);
    
    storage_ = (void*)db;
    return true;
}

bool ProfileManager::LoadProfileFromDb(uint64_t uid, UserProfileData& outProfile) {
    sqlite3* db = (sqlite3*)storage_;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT uid, username, last_checkin_date, continuous_days, "
        "last_danmu_timestamp, created_at, updated_at, keywords_json, danmu_history_json "
        "FROM user_profiles WHERE uid = %llu", uid);
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
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
        if (keywordsJson) outProfile.keywords = JsonToKeywords(keywordsJson);
        
        const char* danmuHistoryJson = (const char*)sqlite3_column_text(stmt, 8);
        if (danmuHistoryJson) outProfile.danmuHistory = JsonToDanmuHistory(danmuHistoryJson);
        
        sqlite3_finalize(stmt);
        return true;
    }
    
    sqlite3_finalize(stmt);
    return false;
}
```

### CaptainCheckInModule.h

```cpp
class CaptainCheckInModule {
    DECLARE_SINGLETON(CaptainCheckInModule)
    
public:
    bool Init();
    void Destroy();
    void PushDanmuEvent(const CaptainDanmuEvent& event);
    void GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback);
    AnswerResult GenerateCheckinAnswerSync(const CheckinEvent& event);
    const UserProfile* GetUserProfile(uint64_t uid) const;
    std::vector<std::string> GetUserTopKeywords(uint64_t uid) const;
    void SetTriggerWords(const std::string& words);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    bool IsCheckinMessage(const std::string& content) const;
    
private:
    int32_t CalculateContinuousDays(uint64_t uid, int32_t checkinDate);
    std::string GenerateAIAnswer(const CheckinEvent& event, const UserProfile* profile);
    std::string GetFallbackAnswer(const CheckinEvent& event);
    void UpdateProfileWithDanmu(UserProfile& profile, const CaptainDanmuEvent& event);
    void ExtractKeywords(UserProfile& profile, const std::string& content);
    void LoadProfileFromDb(uint64_t uid);
    void SaveProfileToDb(const UserProfile& profile);
    
    // 运行时缓存（内存）
    std::map<uint64_t, UserProfile> profiles_;
    std::vector<std::wregex> triggerWordPatterns_;
    std::string triggerWordsStr_;
    std::mutex profilesLock_;
    bool inited_ = false;
    
    std::unique_ptr<IAIChatProvider> aiProvider_;
    std::unique_ptr<ProfileManager> profileManager_;
    
    // 关键词提取
    void ExtractKeywords(UserProfile& profile, const std::string& content);
    void UpdateKeywordFrequency(UserProfile& profile, const std::string& word);
    bool ShouldLearn(const UserProfile& profile, const CaptainDanmuEvent& event);
    bool IsStopWord(const std::string& word) const;
    
    // 防刷屏
    int32_t sameContentCount_ = 0;  // 连续相同内容计数
    std::string lastContent_;
};
```

### 关键词提取实现

```cpp
void CaptainCheckInModule::ExtractKeywords(UserProfile& profile, const std::string& content) {
    std::vector<std::string> words;
    // MixSegment: 融合MP和HMM，效果最好
    jieba_.Cut(content, words, true);
    
    for (const auto& word : words) {
        // 过滤条件
        if (word.length() < 2) continue;  // 单字过滤
        if (IsStopWord(word)) continue;     // 停用词过滤
        
        UpdateKeywordFrequency(profile, word);
    }
}

bool CaptainCheckInModule::IsStopWord(const std::string& word) const {
    // 常用停用词（可扩展为加载 stop_words.utf8）
    static const std::set<std::string> STOP_WORDS = {
        "的", "了", "在", "是", "我", "你", "他", "她", "它",
        "这", "那", "都", "和", "与", "或", "一", "一下",
        "吗", "呢", "吧", "啊", "哦", "嗯", "哈哈", "嘿嘿",
        "可以", "什么", "怎么", "为什么", "有没有", "但是",
        "然后", "所以", "因为", "如果", "虽然", "但是"
    };
    return STOP_WORDS.find(word) != STOP_WORDS.end();
}

void CaptainCheckInModule::UpdateKeywordFrequency(UserProfile& profile, const std::string& word) {
    auto it = std::find_if(profile.keywords.begin(), profile.keywords.end(),
        [&word](const KeywordRecord& r) { return r.word == word; });
    
    if (it != profile.keywords.end()) {
        it->frequency++;
        it->lastSeenTimestamp = GetCurrentTimestamp();
    } else {
        if (profile.keywords.size() >= MAX_KEYWORDS_COUNT) {
            // 移除最低频关键词
            auto minIt = std::min_element(profile.keywords.begin(), profile.keywords.end(),
                [](const KeywordRecord& a, const KeywordRecord& b) { return a.frequency < b.frequency; });
            profile.keywords.erase(minIt);
        }
        profile.keywords.push_back({word, 1, GetCurrentTimestamp()});
    }
    
    // 按频率排序
    std::sort(profile.keywords.begin(), profile.keywords.end(),
        [](const KeywordRecord& a, const KeywordRecord& b) { return a.frequency > b.frequency; });
}
```

### Data Structures

```cpp
struct CaptainDanmuEvent {
    uint64_t uid = 0;
    int32_t guardLevel = 0;
    std::string username;
    std::string content;
    int64_t serverTimestamp = 0;
    int32_t sendDate = 0;
};

struct CheckinEvent {
    uint64_t uid = 0;
    std::string username;
    int32_t continuousDays = 0;
    int32_t checkinDate = 0;
};

struct AnswerResult {
    bool success = false;
    std::string answerContent;
    bool isAiGenerated = false;
    std::string errorMsg;
};

// AI 回复回调类型
using AnswerCallback = std::function<void(const AnswerResult&)>;

// 单个关键词记录
struct KeywordRecord {
    int32_t id;                   // 关键词ID（数据库主键）
    std::string word;              // 关键词（UTF-8）
    int32_t frequency;             // 出现频率
    int64_t lastSeenTimestamp;     // 最后出现时间戳
};

// 运行时用户画像（内存缓存）
struct UserProfile {
    uint64_t uid = 0;
    std::string username;
    std::vector<KeywordRecord> keywords;  // 按频率排序，最多50条
    std::vector<std::pair<int64_t, std::string>> danmuHistory;  // 时间序，最多100条
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;  // 首次学习时间
};

// 数据库用户画像记录（SQLite 存储用）
struct UserProfileData {
    uint64_t uid = 0;
    std::string username;
    std::vector<KeywordRecord> keywords;                      // 运行时使用
    std::vector<std::pair<int64_t, std::string>> danmuHistory;  // 运行时使用
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};
```

## Flow

### 1. 学习阶段（PushDanmuEvent 内部流程）

```
PushDanmuEvent(event)
        │
        ▼
┌───────────────────────────────────────┐
│ 1. ShouldLearn(event) - 学习条件检查   │
│    - 时间窗口：距上次 < 5s → 跳过      │
│    - 内容去重：连续3条相同 → 跳过      │
└───────────────────────────────────────┘
        │ 通过
        ▼
┌───────────────────────────────────────┐
│ 2. UpdateDanmuHistory()               │
│    - 记录 timestamp + content         │
│    - 超过100条 → 移除最早             │
└───────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────┐
│ 3. ExtractKeywords() - cppjieba分词   │
│    - 使用 MixSegment（MP+HMM混合）    │
│    - 过滤停用词（来自 stop_words.utf8）│
│    - 过滤单字词                       │
└───────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────┐
│ 4. UpdateKeywordFrequency()           │
│    - 已存在? frequency++              │
│    - 不存在? 新增                     │
│    - 超过50条? 移除最低频             │
└───────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────┐
│ 5. SaveProfileToDb() - 持久化存储     │
│    - ProfileManager::SaveProfile()    │
│    - 异步写入 SQLite                  │
└───────────────────────────────────────┘
```

### 2. 打卡检测

```
DanmuProcessor::ProcessDanmu()
    → 舰长弹幕事件通过 CaptainDanmuHandler 回调发出
    → CaptainCheckInModule 订阅并处理
        → IsCheckinMessage(content)?
        → 创建 CheckinEvent
        → RecordCheckin()  // 先持久化打卡记录到数据库
        → CalculateContinuousDays()  // 从DB查询打卡记录
        → GenerateCheckinAnswerAsync()
```

**解耦设计**：
- `DanmuProcessor` 通过 `CaptainDanmuHandler` 回调接口发出舰长弹幕事件
- `CaptainCheckInModule` 通过 `DanmuProcessor::AddCaptainDanmuListener()` 订阅事件
- `CaptainDanmuEvent` 结构体定义在 `DanmuProcessor.h` 中
- 两者无直接头文件依赖，实现松耦合

**注意**：必须在生成回复前调用 `RecordCheckin()`，确保打卡记录被持久化。

### 3. AI回复生成 + TTS 播报

```
GenerateCheckinAnswerAsync()
    → API Key 为空? → GetFallbackAnswer()
    → 构建 Prompt（用户名 + 关键词 + 历史发言）
    → 调用 MiniMax API（通过 MiniMaxAIChatProvider）
    → 成功? → 获取文本回复
    → 失败? → GetFallbackAnswer()
    → 文本结果通过 ITTSProvider 进行 TTS 播报
```

**ITTSProvider 播报策略**：
- AI 回复 TTS **独立于弹幕 TTS**，通过 `TTSProviderFactory::Create()` 创建 `ITTSProvider`
- 使用 `AI_PROVIDER` 中的 `tts_provider` 和 `tts_api_key` 配置
- 支持 xiaomi / minimax / sapi（降级）三种 Provider
- Provider 不可用或 API Key 为空时，自动降级到 Windows SAPI
- AI 回复 TTS **受 `enableVoice` 配置影响**：关闭 `enableVoice` 后不进行播报

**AI TTS 播报流程**：
```cpp
void CaptainCheckInModule::PlayCheckinTTS(const std::string& text, ...) {
    auto ttsProvider = TTSProviderFactory::Create(aiProviderJson_);
    if (!ttsProvider || !ttsProvider->IsAvailable()) {
        return;  // 降级到 SAPI
    }
    
    TTSRequest ttsReq;
    ttsReq.text = text;
    ttsReq.style = "开心";  // 固定使用开心风格
    
    ttsProvider->RequestTTS(ttsReq, [&](const TTSResponse& response) {
        if (response.success && !response.audioData.empty()) {
            // 1. 缓存 AI TTS 音频
            TTSCacheManager::Inst()->SaveCheckinAudio(username, response.audioData, timestamp);
            // 2. 通过 TTSManager 播放
            TTSManager::Inst()->PlayAudioData(response.audioData, "mp3");
        }
    });
}
```

**与 TTSManager 的关系**：
| 特性 | AI 回复 TTS | TTSManager 弹幕 TTS |
|------|-------------|-------------------|
| Provider 来源 | `AI_PROVIDER` 配置 | `ConfigManager.ttsEngine` |
| 入口 | `ITTSProvider::RequestTTS()` | `TTSManager::Speak()` |
| 播放 | `TTSManager::PlayAudioData()` | 直接播放 |
| 缓存 | `TTSCacheManager::SaveCheckinAudio()` | `TTSCacheManager::SaveCachedAudio()` |

**AI TTS 音频缓存策略**：
- AI TTS 回复**必须缓存**，使用 `TTSCacheManager` 管理
- 缓存目录：`TempAudio/{YYYYMMDD}/`
- 缓存命名：`打卡_{username}_{timestamp}.mp3`
- 示例：`打卡_舰长A_28475682.mp3`
- 通过 `TTSCacheManager::SaveCheckinAudio()` 保存（独立于通用 TTS 缓存）
- 缓存文件会在 `TTSCacheManager::CleanupOldCache()` 时自动清理（按 `ttsCacheDaysToKeep` 配置）

### 4. 画像加载流程（Init 时）

```
Init()
    → ProfileManager::Inst()->Init()
    → CREATE TABLE IF NOT EXISTS（自动创建表）
    → 内存缓存为空，不主动加载全部画像
    → LoadProfile(uid, outProfile) 时按需从 DB 加载
    → LoadProfileFromDb(uid)
        → ProfileManager::LoadProfileFromDb(uid)
        → 转换为运行时 UserProfileData 结构
```

## AI Prompt 模板

```
用户{username}是一位舰长，今日第{continuousDays}天打卡。
他的发言习惯包含：{keywords}
最近发言：{recentMessages}
请在回复中明确提到用户{username}的姓名，用轻松友好且有点皮的语气回复他的打卡，控制在20字以内。
回复内容需要适合TTS语音播报，避免生僻字和复杂句式。
```

## 事件解耦设计

### 问题说明

**C++ 层没有 `GlobalEventListener` 类**（该类仅存在于 C# 层 `Utils.cs:68`）。需要使用观察者模式在 C++ 层实现事件解耦。

### 解决方案：DanmuProcessor 舰长弹幕回调接口

在 `DanmuProcessor` 中添加 `CaptainDanmuHandler` 回调接口，`CaptainCheckInModule` 通过该接口接收舰长弹幕事件。

**注意**：`DanmuProcessor` 仍然不直接依赖 `CaptainCheckInModule`，只是通过 `std::function` 回调实现松耦合。

```cpp
// DanmuProcessor.h - 添加回调类型和注册方法
public:
    // 舰长弹幕回调
    using CaptainDanmuHandler = std::function<void(const CaptainDanmuEvent&)>;
    void AddCaptainDanmuListener(const CaptainDanmuHandler& handler);
private:
    std::vector<CaptainDanmuHandler> captainDanmuListeners_;
    void NotifyCaptainDanmu(const CaptainDanmuEvent& event);

// DanmuProcessor.cpp - 发出事件
void DanmuProcessor::ProcessDanmu(const DanmuData& danmu) {
    // ... 处理逻辑
    if (danmu.guardLevel >= 3) {
        CaptainDanmuEvent event;
        event.uid = std::stoull(danmu.userId);
        event.guardLevel = danmu.guardLevel;
        event.username = danmu.userName;
        event.content = danmu.message;
        event.serverTimestamp = danmu.timestamp;
        
        NotifyCaptainDanmu(event);  // 通知所有监听者
    }
}

void DanmuProcessor::NotifyCaptainDanmu(const CaptainDanmuEvent& event) {
    for (const auto& handler : captainDanmuListeners_) {
        handler(event);
    }
}

// CaptainCheckInModule.cpp - 注册监听
void CaptainCheckInModule::Init() {
    // ...
    DanmuProcessor::Inst()->AddCaptainDanmuListener([this](const CaptainDanmuEvent& event) {
        if (IsEnabled()) {
            PushDanmuEvent(event);
        }
    });
}
```

### 解耦优势

1. **无直接头文件依赖**：`DanmuProcessor` 不需要 include `CaptainCheckInModule.h`
2. **可插拔**：可以在不修改 DanmuProcessor 的情况下禁用 CaptainCheckInModule
3. **单一职责**：`DanmuProcessor` 只负责弹幕处理，不关心 AI 学习逻辑

### Prompt 构建回退处理

当关键词或历史发言为空时，需要提供默认值：

```cpp
std::string BuildPrompt(const CheckinEvent& event, const UserProfile* profile) {
    std::string keywords;
    if (profile && !profile->keywords.empty()) {
        // 取前5个高频关键词
        int count = std::min(5, (int)profile->keywords.size());
        for (int i = 0; i < count; ++i) {
            if (i > 0) keywords += "、";
            keywords += profile->keywords[i].word;
        }
    } else {
        keywords = "（暂无发言习惯数据）";
    }
    
    std::string recentMessages;
    if (profile && !profile->danmuHistory.empty()) {
        // 取最近3条发言
        int count = std::min(3, (int)profile->danmuHistory.size());
        for (int i = 0; i < count; ++i) {
            if (i > 0) recentMessages += "；";
            recentMessages += profile->danmuHistory[profile->danmuHistory.size() - 1 - i].second;
        }
    } else {
        recentMessages = "（暂无历史发言）";
    }
    
    return "用户" + event.username + "是一位舰长，今日第" + 
           std::to_string(event.continuousDays) + "天打卡。\n" +
           "他的发言习惯包含：" + keywords + "\n" +
           "最近发言：" + recentMessages + "\n" +
           "请在回复中明确提到用户" + event.username + "的姓名，用轻松友好且有点皮的语气回复他的打卡，控制在20字以内。\n" +
           "回复内容需要适合TTS语音播报，避免生僻字和复杂句式。";
}
```

## 回退模板

```
{username}打卡成功！
{username}今日第{continuousDays}天打卡！
```

## File Changes

### New Files

| 文件 | 职责 |
|------|------|
| `MonsterOrderWilds/CaptainCheckInModule.h` | 核心模块头文件 |
| `MonsterOrderWilds/CaptainCheckInModule.cpp` | 核心模块实现 |
| `MonsterOrderWilds/CaptainCheckInModuleTests.cpp` | 单元测试 |
| `MonsterOrderWilds/ProfileManager.h` | 用户画像持久化管理层 |
| `MonsterOrderWilds/ProfileManager.cpp` | ProfileManager 实现 |
| `MonsterOrderWilds/ProfileManagerTests.cpp` | ProfileManager 单元测试 |
| `MonsterOrderWilds/AIChatProvider.h` | AI Chat Provider 接口 |
| `MonsterOrderWilds/MiniMaxAIChatProvider.h` | MiniMax AI Chat Provider 头文件 |
| `MonsterOrderWilds/AIChatProviderFactory.cpp` | AI Chat Provider 工厂实现 |
| `MonsterOrderWilds/MiniMaxAIChatProvider.cpp` | MiniMax AI Chat API 实现 |
| `MonsterOrderWilds/AIChatProviderTests.cpp` | AI Chat Provider 单元测试 |
| `external/sqlite3.h` | SQLite C API 头文件 |
| `external/cppjieba/include/cppjieba/*` | cppjieba 源码（16个头文件） |
| `external/cppjieba/deps/limonp/*` | limonp 依赖库头文件 |
| `MonsterOrderWilds/dict/jieba.dict.utf8` | 最大概率分词词典 |
| `MonsterOrderWilds/dict/hmm_model.utf8` | HMM 模型 |
| `MonsterOrderWilds/dict/stop_words.utf8` | 停用词词典 |
| `MonsterOrderWilds/ITTSProvider.h` | TTS Provider 接口定义 |
| `MonsterOrderWilds/SapiTTSProvider.cpp` | Windows SAPI 降级 Provider 实现 |
| `MonsterOrderWilds/XiaomiTTSProvider.cpp` | Xiaomi TTS 实现 |
| `MonsterOrderWilds/MiniMaxTTSProvider.cpp` | MiniMax TTS 实现 |
| `MonsterOrderWilds/TTSProviderFactory.cpp` | TTS Provider 工厂 |
| `MonsterOrderWilds/TTSProviderTests.cpp` | TTS Provider 单元测试 |
| `openspec/changes/captain-checkin-ai-reply/dict/弹幕习惯词黑白名单配置.txt` | 词典配置说明 |

### Modified Files

| 文件 | 修改内容 |
|------|---------|
| `ConfigManager.h/cpp` | 添加 `enableCaptainCheckinAI` 和 `checkinTriggerWords` 字段 |
| `ConfigFieldRegistry.cpp` | 注册新字段 |
| `CredentialsManager.h/cpp` | 添加 `GetAI_PROVIDER()`，扩展 JSON 解析支持 AI_PROVIDER 字段 |
| `DataBridgeWrapper.h` | `ConfigProxy` 添加 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords` 属性 |
| `DataBridgeExports.h` | 导出新字段 |
| `DataStructures.cs` | `ConfigDataSnapshot` 添加 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords` |
| `Utils.cs` | `MainConfig` 添加 `ENABLE_CAPTAIN_CHECKIN_AI` 和 `CHECKIN_TRIGGER_WORDS` 属性 |
| `ProxyClasses.cs` | `ConfigProxy` 添加 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords` 属性 |
| `ToolsMain.cs` | `ConfigChanged()` 处理 `ENABLE_CAPTAIN_CHECKIN_AI` 和 `CHECKIN_TRIGGER_WORDS` |
| `ConfigWindow.xaml/cs` | 新增"舰长打卡AI" Tab |
| `DanmuProcessor.cpp` | 添加 CaptainDanmuHandler 回调接口 |

## Dependencies

- **SQLite** - SQLite C API（https://www.sqlite.org/）
- **cppjieba** - 中文分词库（头文件-only，集成到项目中）
- **limonp** - cppjieba 依赖库（头文件-only）
- **词典文件** - jieba.dict.utf8, hmm_model.utf8, stop_words.utf8
- MiniMax API（复用 `MimoTTSClient` 网络请求机制）
- JSON 解析（复用 `nlohmann/json`）
- 正则表达式（`std::wregex`）

## 初始化顺序

模块初始化顺序必须遵循以下依赖关系：

**第一步：通过 `DataBridge::Initialize()` 初始化基础模块**（C# 层调用 `DataBridge_Initialize()` P/Invoke）

```
1. ConfigFieldRegistry::RegisterAll()  → 注册所有配置字段及其回调
2. ConfigManager::LoadConfig()         → 加载配置
3. MonsterDataManager::LoadJsonData()   → 加载怪物数据
4. PriorityQueueManager::LoadList()    → 加载队列数据
5. CredentialsManager::Init()          → 加载凭据（AI_PROVIDER）
6. DanmuProcessor 同步配置             → 同步过滤条件
```

**第二步：在 `DataBridge::Initialize()` 之后，单独初始化 CaptainCheckInModule**

```
7. ProfileManager::Inst()->Init()            → 初始化 SQLite 数据库（无参数）
8. DanmuProcessor::Inst()->Init()            → 初始化弹幕处理器（此时可以开始接收弹幕）
9. CaptainCheckInModule::Inst()->Init()      → 初始化 cppjieba，向 DanmuProcessor 注册舰长弹幕回调
```

**注意**：
- `ProfileManager` 使用 `DEFINE_SINGLETON(ProfileManager)` 宏
- `CaptainCheckInModule` 使用 `DEFINE_SINGLETON(CaptainCheckInModule)` 宏
- 需要在 `DataBridgeExports.cpp` 中添加 `CaptainCheckInModule_Initialize()` 导出函数，供 C# 层在 `DataBridge_Initialize()` 之后调用
- `CaptainCheckInModule::Init()` 必须在 `DanmuProcessor::Init()` 之后调用，因为需要在 `Init()` 中向 DanmuProcessor 注册监听器
- `DanmuProcessor` 先初始化，CaptainCheckInModule 后注册监听器，这是正确的时序（监听器在事件发生前注册即可）

## 数据库迁移策略

**当前策略**：使用 `CREATE TABLE IF NOT EXISTS` 自动创建表，不自动迁移 schema 变更。

**限制**：
- 首次安装时自动创建表
- 表结构变更（如添加新字段）需要手动处理

**未来扩展**：
- 如果需要支持数据库 schema 迁移，可以：
  1. 在 `ProfileManager` 中记录 schema 版本号
  2. 检测版本号变化时执行 ALTER TABLE 语句
  3. 或者在升级时删除旧数据库并重新创建（会丢失用户数据）

**当前实现建议**：由于是第一版设计，暂不实现复杂迁移策略。如果后续需要变更数据库结构，建议通过版本号检测并提示用户重新开始。

## 数据库文件位置

```
MonsterOrderWilds_configs/
├── MainConfig.cfg          # 现有配置
├── credentials.dat         # 现有凭据
└── captain_profiles.db     # 用户画像数据库（新增）
```

## 配置说明

| 字段 | 类型 | 说明 | 管理方 |
|------|------|------|--------|
| `ENABLE_CAPTAIN_CHECKIN_AI` | bool | AI学习功能总开关，默认 true | C++ ConfigManager / C# 配置层 |
| `CHECKIN_TRIGGER_WORDS` | string | 打卡触发词，逗号分隔，默认 "打卡,签到" | C++ ConfigManager / C# 配置层 |
| `AI_PROVIDER` | credential | 所有 AI Provider 和 API Key，JSON格式 | C++ CredentialsManager |

**说明**：
- `AI_PROVIDER` 是 credential 字段，由 C++ `CredentialsManager` 独立管理，**复用现有的 HMAC 加密机制**存储在 `credentials.dat` 中，不经过 C# 配置层
- `ENABLE_CAPTAIN_CHECKIN_AI` 和 `CHECKIN_TRIGGER_WORDS` 是普通配置字段，由 C++ ConfigManager 和 C# 配置层共同管理
- C# 配置层字段使用 SCREAMING_SNAKE 命名风格（如 `ENABLE_CAPTAIN_CHECKIN_AI`）

**Credential JSON 格式**（存储在 `credentials.dat` 中）：
```json
{
  "tts_provider": "xiaomi",
  "tts_api_key": "xxxx",
  "chat_provider": "minimax",
  "chat_api_key": "xxx"
}
```

**存储机制**：
- `credentials.dat` 使用 HMAC-SHA256 加密存储，盐值为 `SALT`
- `AI_PROVIDER` 字段作为 JSON 字符串存储在 credentials.dat 中
- 复用现有 `CredentialsManager::LoadCredentials()` 和 `CredentialsManager::GetAI_PROVIDER()` 接口

## MiniMax AI Chat API 调用详情（MiniMaxAIChatProvider）

### API 端点
- **Endpoint**: `api.minimaxi.com`
- **Port**: 443
- **Path**: `/v1/text/chatcompletion_v2`
- **Method**: POST

### 请求头
```
Content-Type: application/json
Authorization: Bearer {CHAT_AI_API_KEY}
```

### 请求体
```json
{
  "model": "M2-her",
  "messages": [
    {"role": "user", "content": "{prompt}"}
  ]
}
```

### 响应解析
```json
{
  "choices": [
    {
      "finish_reason": "stop",
      "message": {
        "content": "AI生成的文本回复"
      }
    }
  ]
}
```

## TTS AI API 调用详情

### Xiaomi TTS API（XiaomiTTSProvider）

**API 端点**:
- **Endpoint**: `api.xiaomimimo.com`
- **Port**: 443
- **Path**: `/v1/chat/completions`
- **Method**: POST

**请求头**:
```
Content-Type: application/json
Authorization: Bearer {TTS_AI_API_KEY}
```

**请求体**:
```json
{
  "model": "mimo-v2-tts",
  "messages": [
    {
      "role": "assistant",
      "content": "<style>开心</style>待合成内容"
    }
  ],
  "audio": {
    "voice": "mimo_default",
    "format": "wav"
  }
}
```

**可选音色**:
| 音色名 | voice参数 |
|--------|---------|
| MiMo-默认 | mimo_default |
| MiMo-中文女声 | default_zh |
| MiMo-英文女声 | default_en |

**风格控制**:
- 整体风格：将 `<style>风格</style>` 置于目标文本开头
- 风格示例：开心、东北话、粤语、唱歌、夹子音
- 细粒度控制：支持音频标签如 `(紧张，深呼吸)`, `(语速加快)`

### MiniMax TTS API（MiniMaxTTSProvider）

**API 端点**:
- **Endpoint**: `api.minimaxi.com`
- **Port**: 443
- **Path**: `/v1/t2a_v2`
- **Method**: POST

**请求头**:
```
Content-Type: application/json
Authorization: Bearer {TTS_AI_API_KEY}
```

**请求体**:
```json
{
  "model": "speech-2.8-hd",
  "text": "需要合成语音的文本",
  "stream": false,
  "voice_setting": {
    "voice_id": "male-qn-qingse",
    "speed": 1,
    "vol": 1,
    "pitch": 0,
    "emotion": "happy"
  },
  "audio_setting": {
    "sample_rate": 32000,
    "bitrate": 128000,
    "format": "mp3",
    "channel": 1
  }
}
```

**响应解析**:
```json
{
  "data": {
    "audio": "hex编码的audio",
    "status": 2
  }
}
```

## 扩展性设计

### ITTSProvider（TTS 语音合成）
| Provider | 实现类 | 说明 |
|----------|--------|------|
| xiaomi | XiaomiTTSProvider | Xiaomi TTS API |
| minimax | MiniMaxTTSProvider | MiniMax TTS API |
| sapi | SapiTTSProvider | Windows SAPI（降级 fallback） |

### IAIChatProvider（文本对话）
| Provider | 实现类 | 说明 |
|----------|--------|------|
| minimax | MiniMaxAIChatProvider | MiniMax 文本对话 API |

## Error Handling

- AI调用失败 → 回退固定模板
- API Key为空 → 使用固定模板
- 数据库/文件异常 → 纯内存模式
- 线程安全 → `std::mutex` 保护 `profiles_`
