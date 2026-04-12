# 舰长打卡AI回复系统 - 主规格

## 修复记录

### 2026-04-12: TempAudio目录只保存签到TTS

**修复的问题**：
- **一般弹幕 TTS 误存 TempAudio** (`TextToSpeech.cpp`)
  - 问题：`TempAudio/{YYYYMMDD}/` 目录原本设计为只保存签到 AI 回复 TTS，但一般弹幕播报也调用了 `SaveCachedAudio()` 写入该目录
  - 修复：移除一般弹幕的 `SaveCachedAudio()` 调用，改为直接播放后丢弃，不再缓存

**相关文件修改**：
- `MonsterOrderWilds/TextToSpeech.cpp` - 移除一般弹幕的 `SaveCachedAudio()` 调用

---

### 2026-04-12: AI回复和TTS播放问题修复

**修复的问题**：

1. **AI Chat Provider `IsAvailable()` 返回值错误** (`DeepSeekAIChatProvider.h`, `MiniMaxAIChatProvider.h`)
   - 问题：`IsAvailable()` 返回的是上次 API 调用结果（`available_`），而不是检查 API key 是否有效
   - 修复：改为 `return !apiKey_.empty()`，只要 API key 非空就返回 true

2. **TTS Provider `IsAvailable()` 返回值错误** (`TTSProvider.h`, `XiaomiTTSProvider.cpp`, `MiniMaxTTSProvider.cpp`)
   - 问题：同上，`IsAvailable()` 返回 `available_` 而不是检查 API key
   - 修复：改为 `return !apiKey_.empty()`

 3. **签到 TTS 播放失败** (`CaptainCheckInModule.cpp`, `TextToSpeech.cpp`, `TextToSpeech.h`)
   - 问题：`PlayCheckinTTS` 使用 `TTSProviderFactory` 创建 provider（可能是 xiaomi），但 XiaomiTTS 返回的音频格式不被 MCI 正确支持；且之前未调用 `SaveCheckinAudio()` 保存音频
   - 修复：新增 `TTSManager::SpeakCheckinTTS()` 方法，使用 `MimoTTSClient`（与弹幕 TTS 相同方式），签到 TTS 播放成功后调用 `SaveCheckinAudio()` 保存音频到 `打卡_{username}_{timestamp}.mp3`；时间戳使用 `GetTickCount64()` 与 `SaveCachedAudio()` 保持一致

 4. **签到弹幕触发普通 TTS** (`TextToSpeech.cpp`)
   - 问题：弹幕 "签到" 或 "打卡" 被 `HandleSpeekDm` 处理后加入 `NormalMsgQueue`，导致播放弹幕内容 "签到" 而不是 AI 回复
   - 修复：在 `HandleSpeekDm` 中检测签到消息（`msg == "签到" || msg == "打卡"`），跳过普通 TTS 队列处理

5. **AudioPlayer 死锁** (`AudioPlayer.cpp`)
   - 问题：`Play()` 获取自旋锁后，如果 `playing==true` 会调用 `Stop()`，而 `Stop()` 尝试获取同一个锁导致死锁
   - 修复：先检查 `playing` 状态并释放锁，再执行 MCI stop/close 命令

**相关文件修改**：
- `MonsterOrderWilds/DeepSeekAIChatProvider.h` - `IsAvailable()` 返回 `!apiKey_.empty()`
- `MonsterOrderWilds/MiniMaxAIChatProvider.h` - 同上
- `MonsterOrderWilds/TTSProvider.h` - `XiaomiTTSProvider` 和 `MiniMaxTTSProvider` 的 `IsAvailable()` 改为内联实现
- `MonsterOrderWilds/XiaomiTTSProvider.cpp` - 删除 `IsAvailable()` 定义
- `MonsterOrderWilds/MiniMaxTTSProvider.cpp` - 删除 `IsAvailable()` 定义
- `MonsterOrderWilds/TextToSpeech.h` - 新增 `SpeakCheckinTTS()` 方法声明
- `MonsterOrderWilds/TextToSpeech.cpp` - 实现 `SpeakCheckinTTS()`，修改 `HandleSpeekDm()` 跳过签到消息
- `MonsterOrderWilds/CaptainCheckInModule.cpp` - `PlayCheckinTTS()` 改用 `SpeakCheckinTTS()`
- `MonsterOrderWilds/AudioPlayer.cpp` - 修复 `Play()` 死锁问题

---

### 2026-04-12 (早期): 初始化和死锁问题修复

**修复的问题**：

1. **CaptainCheckInModule 未初始化** (`DataBridge.h`)
   - 问题：`DataBridge::Initialize()` 没有调用 `CaptainCheckInModule::Init()`，导致签到模块完全未初始化
   - 修复：在 `DataBridge::Initialize()` 中添加 `ProfileManager::Inst()->Init()`、`GetDanmuProcessor()->Init()` 和 `CaptainCheckInModule::Inst()->Init()`

2. **词典文件缺失** (`CaptainCheckInModule.cpp`)
   - 问题：Jieba 构造函数需要 5 个参数（含 `idfPath`），但代码只传了 4 个，导致 `idfAverage_ > 0.0` 断言失败
   - 修复：添加 `idf.utf8` 路径参数和文件检查

3. **死锁问题** (`CaptainCheckInModule.cpp`)
   - 问题：`PushDanmuEvent` 持有 `profilesLock_` 时调用 `GenerateCheckinAnswerSync`，而后者也会尝试获取 `profilesLock_`，导致死锁
   - 修复：修改 `GenerateCheckinAnswerSync` 接受可选的 `profile` 指针参数，避免重复加锁

4. **dict 文件未复制到输出目录** (`MonsterOrderWilds.vcxproj`)
   - 问题：Debug 构建时 dict 文件没有复制到 `x64\Debug\dict\`，导致运行时找不到文件
   - 修复：添加 PostBuildEvent 复制 `dict\*` 到输出目录

**相关文件修改**：
- `MonsterOrderWilds/DataBridge.h` - 添加初始化调用
- `MonsterOrderWilds/CaptainCheckInModule.cpp` - 添加 idf 支持，修复死锁
- `MonsterOrderWilds/CaptainCheckInModule.h` - 更新函数签名
- `MonsterOrderWilds/MonsterOrderWilds.vcxproj` - 添加 PostBuildEvent

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
- 用户画像持久化到 SQLite 数据库（使用 SQLite C API）

**详见**: `keyword-extraction-spec.md`

### FR-2: 打卡检测

- 支持可配置的触发词列表（**默认"打卡,签到"**）
- 触发词用逗号分隔，支持中文和英文
- 检测到打卡消息时触发 AI 回复生成
- ⚠️ **重要**：如果配置为空或被删除，打卡检测功能将完全失效

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
- **AI 回复通过 TTS 播报**，使用 `ConfigManager.ttsEngine` 配置（mimo/sapi/auto）

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
调用 `TTSCacheManager::SaveCheckinAudio()` 缓存音频（`打卡_{username}_{timestamp}.mp3`）
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
    
    void PushDanmuEvent(const CaptainDanmuEvent& event);
    void GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback);
    AnswerResult GenerateCheckinAnswerSync(const CheckinEvent& event);
    
    const UserProfile* GetUserProfile(uint64_t uid) const;
    std::vector<std::string> GetUserTopKeywords(uint64_t uid) const;
    void SetTriggerWords(const std::string& words);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    bool IsCheckinMessage(const std::string& content) const;
};
```

## 依赖规格

- `keyword-extraction-spec.md` - 关键词提取模块规格（含安装包配置）
- `ai-chat-provider-spec.md` - AI Chat Provider 接口规格
- `tts-provider-spec.md` - TTS Provider 接口规格（**必须实现**）
- **SQLite** - SQLite C API（https://www.sqlite.org/）

## 编译宏

| 宏名 | 默认值 | 说明 |
|------|--------|------|
| `TEST_CAPTAIN_REPLY_LOCAL` | `_DEBUG` | 本地测试宏。设为 1 时跳过舰长检测（guard_level >= 3）和防刷屏检查，方便本地测试 captain-checkin-ai-reply 功能。**在 Debug 配置下自动启用**，Release 配置下为 0 |

**注意**：
- `TEST_CAPTAIN_REPLY_LOCAL` 仅用于本地开发调试，**不要提交到正式版本**
- 宏开启时，词典文件（`dict/*.utf8`）必须存在，否则会输出明确错误日志
- 已修复问题：词典文件检查现在包含 `idf.utf8`（之前遗漏导致 cppjieba 初始化失败）

## 数据库

- 数据库文件：`MonsterOrderWilds_configs/captain_profiles.db`
- 使用 SQLite C API 管理，存储用户画像和打卡记录
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
| `dict/idf.utf8` | `{AppRoot}/dict/idf.utf8` |
| `dict/stop_words.utf8` | `{AppRoot}/dict/stop_words.utf8` |
| `dict/user.dict.utf8` | `{AppRoot}/dict/user.dict.utf8` |
| `dict/弹幕习惯词黑白名单配置.txt` | `{AppRoot}/dict/弹幕习惯词黑白名单配置.txt` |

**注意**：
- `captain_profiles.db` 在运行时自动创建，无需打包
- `弹幕习惯词黑白名单配置.txt` 是配置说明文档
- **所有 dict 文件都必须在 Debug 构建时复制到输出目录**（通过 vcxproj 的 PostBuildEvent 配置）

详见 `keyword-extraction-spec.md` 的「安装包配置」章节。
