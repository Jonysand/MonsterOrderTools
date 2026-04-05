# 舰长打卡AI回复系统 - 主规格

## 概述

学习舰长发言习惯，当舰长发送含打卡触发词的弹幕时，基于其历史发言习惯生成个性化AI回复。

## 功能需求

### FR-1: 舰长发言学习

- 仅处理 guard_level ≥ 3 的舰长弹幕
- 记录弹幕内容到用户画像的 danmuHistory（最多100条，超出移除最早）
- 关键词存入 keywords（按频率排序，最多50条）
- 防刷屏机制：
  - 时间窗口：同用户 5 秒内只学习第一条
  - 内容去重：连续 3 条内容相同则跳过
- 用户画像持久化到 SQLite 数据库（使用 sqlite_orm）

**详见**: `keyword-extraction-spec.md`

### FR-2: 打卡检测

- 支持可配置的触发词列表（默认"打卡,签到"）
- 触发词用逗号分隔，支持中文和英文
- 检测到打卡消息时触发 AI 回复生成

### FR-3: 连续打卡天数计算

- 模块自己计算连续打卡天数
- 根据上次打卡日期和当前日期判断是否连续
- 连续天数作为 AI 回复 Prompt 的一部分

### FR-4: AI 回复生成

- 调用 MiniMax 文本对话 API 生成个性化回复
- API 端点: `api.minimaxi.com/v1/text/chatcompletion_v2`
- Model: `M2-her`
- Prompt 包含：用户名、连续打卡天数、用户关键词、最近发言
- API 调用失败时回退到固定模板
- API Key 为空时直接使用固定模板
- **AI 回复通过 TTS 播报**，使用 `AI_PROVIDER` 中的 `tts_provider` 配置

**详见**: `ai-chat-provider-spec.md`

### FR-5: 固定回复模板

- `"{username}打卡成功！"`
- `"{username}今日第{continuousDays}天打卡！"`

## 配置字段

| JSON字段 | C++字段 | C#字段 | 类型 | 默认值 |
|----------|---------|--------|------|--------|
| ENABLE_CAPTAIN_CHECKIN_AI | enableCaptainCheckinAI | ENABLE_CAPTAIN_CHECKIN_AI | bool | true |
| CHECKIN_TRIGGER_WORDS | checkinTriggerWords | CHECKIN_TRIGGER_WORDS | string | "打卡,签到" |

**说明**：
- `AI_PROVIDER` 是 credential 字段，由 C++ `CredentialsManager` 独立管理，**复用现有的 HMAC 加密机制**存储在 `credentials.dat` 中，不经过 C# 配置层
- `ENABLE_CAPTAIN_CHECKIN_AI` 控制整个功能的开启/关闭，关闭时不进行学习、统计、AI回复等任何操作
- C# 配置层处理 `ENABLE_CAPTAIN_CHECKIN_AI` 和 `CHECKIN_TRIGGER_WORDS`
- C# 字段使用 SCREAMING_SNAKE 命名风格

**Credential JSON 格式**（C++ 独立处理）:
```json
{
  "tts_provider": "xiaomi",
  "tts_api_key": "xxxx",
  "chat_provider": "minimax",
  "chat_api_key": "xxx"
}
```

## 数据结构

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

struct KeywordRecord {
    std::string word;              // 关键词（UTF-8）
    int32_t frequency;           // 出现频率
    int64_t lastSeenTimestamp;   // 最后出现时间戳
};

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

// 数据库用户画像记录（SQLite 存储）
struct UserProfileRecord {
    uint64_t uid = 0;                     // 主键
    std::string username;
    std::string keywordsJson;              // JSON: [{"word":"太刀","freq":5,"ts":123456},...]
    std::string danmuHistoryJson;          // JSON: [{"ts":123456,"content":"内容"},...]
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

// 打卡记录条目
struct CheckinRecordEntry {
    int64_t id = 0;
    uint64_t uid = 0;
    int32_t checkinDate = 0;
    int64_t createdAt = 0;
};
```

## 数据流

### 学习阶段

```
弹幕接收 (guard_level >= 3)
    ↓
DanmuProcessor 通过 CaptainDanmuHandler 回调发出事件
    ↓
CaptainCheckInModule::AddCaptainDanmuListener() 订阅处理
    ↓
PushDanmuEvent()
    ↓
ShouldLearn() - 学习条件检查
    ↓
UpdateDanmuHistory() - 记录弹幕历史
    ↓
ExtractKeywords() - 分词提取关键词
    ↓
UpdateKeywordFrequency() - 更新关键词频率
```

### 打卡阶段

```
弹幕接收
    ↓
IsCheckinMessage() 检测打卡
    ↓
RecordCheckin() 持久化打卡记录
    ↓
CalculateContinuousDays() 计算天数
    ↓
GenerateCheckinAnswerAsync() 生成回复
    ↓
获取AI回复文本 → ITTSProvider TTS播报（自动处理引擎回退）
    ↓
调用 `TTSCacheManager::SaveCachedAudio()` 缓存音频（`打卡_{username}_{timestamp}.mp3`）
```

**事件解耦**：`DanmuProcessor` 与 `CaptainCheckInModule` 通过 `CaptainDanmuHandler` 回调接口解耦，无直接头文件依赖。

> **详细设计**：见 `design.md` 第 620-678 行「事件解耦设计」章节。

**注意**：AI 回复的 TTS 播报**受 `enableVoice` 配置影响**。关闭 `enableVoice` 后不进行播报。

## AI Prompt 模板

```
用户{username}是一位舰长，今日第{continuousDays}天打卡。
他的发言习惯包含：{keywords}
最近发言：{recentMessages}
请在回复中明确提到用户{username}的姓名，用轻松友好且有点皮的语气回复他的打卡，控制在20字以内。
回复内容需要适合TTS语音播报，避免生僻字和复杂句式。
```

## 接口设计

### CaptainCheckInModule

```cpp
class CaptainCheckInModule {
public:
    static CaptainCheckInModule& Inst();
    bool Init();
    void Destroy();
    
    void PushDanmuEvent(const CaptainDanmuEvent& event);
    void GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback);
    AnswerResult GenerateCheckinAnswerSync(const CheckinEvent& event);
    
    const UserProfile* GetUserProfile(uint64_t uid) const;
    std::vector<std::string> GetUserTopKeywords(uint64_t uid) const;
    void SetTriggerWords(const std::string& words);
    bool IsCheckinMessage(const std::string& content) const;
};
```

## 依赖规格

- `keyword-extraction-spec.md` - 关键词提取模块规格（含安装包配置）
- `ai-chat-provider-spec.md` - AI Chat Provider 接口规格
- `tts-provider-spec.md` - TTS Provider 接口规格（**必须实现**）
- **sqlite_orm** - SQLite ORM 库（头文件-only，https://github.com/fnc12/sqlite_orm）

## 数据库

- 数据库文件：`MonsterOrderWilds_configs/captain_profiles.db`
- 使用 sqlite_orm 管理，存储用户画像和打卡记录
- 数据库在首次初始化时自动创建（`sync_schema`）

**数据库 Schema**（与 `UserProfileRecord` 和 `CheckinRecordEntry` 结构体一一对应）：

```sql
CREATE TABLE user_profiles (
    uid INTEGER PRIMARY KEY,
    username TEXT NOT NULL,
    keywords TEXT,
    danmu_history TEXT,
    last_checkin_date INTEGER DEFAULT 0,
    continuous_days INTEGER DEFAULT 0,
    last_danmu_timestamp INTEGER DEFAULT 0,
    created_at INTEGER DEFAULT 0,
    updated_at INTEGER DEFAULT 0
);

CREATE TABLE checkin_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uid INTEGER NOT NULL,
    checkin_date INTEGER NOT NULL,
    created_at INTEGER NOT NULL,
    UNIQUE(uid, checkin_date)
);

CREATE INDEX idx_checkin_uid_date ON checkin_records(uid, checkin_date);
```

> **详细设计**：见 `design.md` 第 132-159 行「数据库结构」章节。

## 安装包配置

**必须打包到安装包的文件**：

| 文件 | 目标路径 |
|------|----------|
| `dict/jieba.dict.utf8` | `{AppRoot}/dict/jieba.dict.utf8` |
| `dict/hmm_model.utf8` | `{AppRoot}/dict/hmm_model.utf8` |
| `dict/stop_words.utf8` | `{AppRoot}/dict/stop_words.utf8` |
| `dict/弹幕习惯词黑白名单配置.txt` | `{AppRoot}/dict/弹幕习惯词黑白名单配置.txt` |

**注意**：
- `captain_profiles.db` 在运行时自动创建，无需打包
- `弹幕习惯词黑白名单配置.txt` 是配置说明文档

详见 `keyword-extraction-spec.md` 的「安装包配置」章节。
