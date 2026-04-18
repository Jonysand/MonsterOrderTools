# 舰长打卡AI回复系统 - 主规格

## 修复记录

### 2026-04-18: 修复舰长点怪默认优先排序问题

**修复的问题**：

1. **舰长点怪被默认优先排序** (`DanmuProcessor.cpp`)
   - 问题：`node.priority = danmu.guardLevel > 0;` 把舰长身份错误地映射为优先标志，导致所有舰长点怪都自动排到非舰长前面
   - 修复：改为 `node.priority = false;`，优先权应由用户明确说"优先"关键词才能获得
   - 排序行为：舰长点怪现在按时间顺序排列，只有明确说"优先"/"插队"等关键词才会插队

**相关文件修改**：
- `MonsterOrderWilds/DanmuProcessor.cpp:190` - 入队时 priority 默认设为 false

---

### 2026-04-18: 修复非舰长打卡和点怪过滤问题

**修复的问题**：

1. **非舰长+粉丝牌用户无法触发打卡** (`DanmuProcessor.cpp`)
   - 问题：`PassesFilter` 中包含 `onlySpeekGuardLevel` 和 `onlySpeekPaidGift` 过滤，当配置"播报至少等级=舰长"时，非舰长用户被过滤掉，导致 `CaptainDanmuEvent` 通知无法发出
   - 修复：将 `NotifyCaptainDanmu` 调用提前到 `PassesFilter` 之前，使打卡功能独立于播报过滤配置
   - 触发条件 `guardLevel != 0 || hasMedal` 保持不变

2. **非舰长打卡出现两次气泡** (`CaptainCheckInModule.cpp`)
   - 问题：条件 `!enableVoice || guardLevel == 0` 中，`guardLevel == 0` 对非舰长恒为 true，导致 `enableVoice=true` 时既直接显示气泡，又调用 `PlayCheckinTTS`（TTS 回调再次显示）
   - 修复：移除 `guardLevel == 0` 条件，改为 `if (!enableVoice) { 直接显示 } else { TTS回调显示 }`
   - 非舰长打卡行为：enableVoice=false 立即显示气泡，enableVoice=true TTS 播放时显示气泡（与舰长一致）

3. **点怪被错误过滤** (`DanmuProcessor.cpp`)
   - 问题：`onlySpeekGuardLevel` 和 `onlySpeekPaidGift` 是"播报过滤"配置，却错误地放在 `PassesFilter` 中影响点怪逻辑
   - 修复：从 `PassesFilter` 中移除这两个过滤条件，只保留 `onlyMedalOrder`（仅粉丝牌可点怪）
   - 播报过滤保留在 `ShouldSpeak` 中，只影响语音播报，不影响点怪

**行为变更**：

| 场景 | enableVoice=false | enableVoice=true |
|------|------------------|-----------------|
| 非舰长+有粉丝牌+首次打卡 | 立即显示气泡 | TTS 播放时显示气泡 |
| 非舰长+有粉丝牌+重复打卡 | 立即显示气泡 | TTS 播放时显示气泡 |
| 舰长+首次打卡 | 立即显示气泡 | TTS 播放时显示气泡 |
| 舰长+重复打卡 | 立即显示气泡 | TTS 播放时显示气泡 |

**配置影响说明**：

| 配置项 | 影响范围 | 说明 |
|--------|---------|------|
| `onlyMedalOrder` | 点怪 | 仅粉丝牌可点怪 |
| `onlySpeekWearingMedal` | 播报 | 仅播报佩戴粉丝牌的弹幕 |
| `onlySpeekGuardLevel` | 播报 | 仅播报指定舰长等级的弹幕 |
| `onlySpeekPaidGift` | 播报 | 仅播报付费礼物弹幕 |

**相关文件修改**：
- `MonsterOrderWilds/DanmuProcessor.cpp` - `NotifyCaptainDanmu` 提前到 `PassesFilter` 之前；`PassesFilter` 移除 `onlySpeekGuardLevel` 和 `onlySpeekPaidGift`
- `MonsterOrderWilds/CaptainCheckInModule.cpp` - 非舰长打卡移除 `guardLevel == 0` 条件

---

### 2026-04-18: 修复气泡显示依赖语音播报的问题

**修复的问题**：

1. **气泡显示被错误绑定到 TTS 播放流程** (`CaptainCheckInModule.cpp`)
   - 问题：重复打卡和舰长首次打卡的气泡显示仅在 `enableVoice=true` 时生效，因为代码通过 `PlayCheckinTTS` 间接触发 `g_checkinTTSPlayCallback` 来显示气泡
   - 修复：
     - 当 `enableVoice=false` 时，直接调用 `g_aiReplyCallback` 显示气泡
     - 当 `enableVoice=true` 时，保持原有行为：TTS 播放时通过 `g_checkinTTSPlayCallback` 同步显示气泡
   - 非舰长首次打卡原本总是直接调用 `g_aiReplyCallback`，现在也改为仅在 `enableVoice=false` 时直接显示，开启语音时改为通过 TTS 回调同步显示

2. **移除 TextToSpeech.cpp 中多余的 g_checkinTTSPlayCallback 调用**
   - 问题：非舰长首次打卡在开启语音时，既直接调用了 `g_aiReplyCallback`，又通过 TTS 回调调用了 `g_checkinTTSPlayCallback`，导致气泡重复显示
   - 修复：保留 `g_checkinTTSPlayCallback` 机制，但从 `CaptainCheckInModule` 层面控制，仅在需要时调用

**行为变更**：

| 场景 | enableVoice=false | enableVoice=true |
|------|------------------|-----------------|
| 重复打卡 | 立即显示气泡 | TTS 播放时显示气泡 |
| 舰长首次打卡 | 立即显示气泡 | TTS 播放时显示气泡 |
| 非舰长首次打卡 | 立即显示气泡 | TTS 播放时显示气泡 |

**相关文件修改**：
- `MonsterOrderWilds/CaptainCheckInModule.cpp` - 添加 enableVoice 分支逻辑
- `MonsterOrderWilds/TextToSpeech.cpp` - 恢复 g_checkinTTSPlayCallback 调用，保持与语音同步

---

### 2026-04-16: 非舰长+有粉丝牌打卡功能

**新增功能**：

1. **打卡触发条件扩展**
   - 原条件：`guardLevel != 0`（仅舰长）
   - 新条件：`guardLevel != 0 || hasMedal`（舰长或有粉丝牌均可触发）

2. **学习机制限制**
   - 仅舰长（`guardLevel > 0`）才进行弹幕学习和关键词提取
   - 非舰长用户不进行学习，但可正常打卡

3. **非舰长打卡回复**
   - 舰长：调用 AI 生成回复 + TTS 播报 + 气泡显示
   - 非舰长+有粉丝牌：固定回复模板 + 气泡显示（无 TTS）

4. **固定回复模板更新**
   - 原格式：`"{username}今日第{continuousDays}天打卡！"`
   - 新格式：`"{username}连续第{continuousDays}天打卡！"`

5. **数据结构变更**
   - `CaptainDanmuEvent` 添加 `hasMedal` 字段（是否佩戴粉丝牌）

**相关文件修改**：
- `MonsterOrderWilds/CaptainCheckInModule.h` - 添加 `hasMedal` 字段
- `MonsterOrderWilds/CaptainCheckInModule.cpp` - 学习逻辑、TTS 逻辑、回复生成逻辑
- `MonsterOrderWilds/DanmuProcessor.h` - 添加 `hasMedal` 字段
- `MonsterOrderWilds/DanmuProcessor.cpp` - 传递 `hasMedal` 值
- `MonsterOrderWilds/DataBridgeExports.h` - 导出 `g_aiReplyCallback` 供气泡显示使用

---

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

- **仅处理舰长弹幕**（`guardLevel > 0`）
- 非舰长用户（有粉丝牌）不进行学习，但可正常打卡
- 记录弹幕内容到用户画像的 danmuHistory（最多100条，超出移除最早）
- 关键词存入 keywords（按频率排序，最多50条）
- 防刷屏机制：
  - 时间窗口：同用户 5 秒内只学习第一条
  - 内容去重：连续 3 条内容相同则跳过
- 用户画像持久化到 SQLite 数据库（使用 SQLite C API）

**详见**: `keyword-extraction-spec.md`

### FR-2: 打卡检测

- **触发条件**：`guardLevel != 0 || hasMedal`（舰长或有粉丝牌均可触发）
- **匹配规则**：触发词必须与弹幕内容 **精确匹配（exact match）**，包含触发词但不完全相同时不触发
  - ✅ 触发示例：弹幕="打卡"，触发词="打卡,签到" → 触发打卡
  - ❌ 不触发示例：弹幕="我来打卡了"，触发词="打卡" → 不触发（包含但不精确匹配）
  - ❌ 不触发示例：弹幕="打卡签到"，触发词="打卡,签到" → 不触发（是多个触发词的组合，非精确匹配单个触发词）
- 支持可配置的触发词列表（**默认"打卡,签到"**）
- 触发词用逗号分隔，支持中文和英文，自动去除前后空格
- 匹配不区分大小写
- 检测到打卡消息时触发回复生成
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
- **非舰长用户不走 AI 回复，直接使用固定模板**

**详见**: `ai-chat-provider-spec.md`

### FR-5: 固定回复模板

- `"{username}打卡成功！"`
- `"{username}连续第{continuousDays}天打卡！"`

### FR-6: 打卡行为与气泡显示时机

| 场景 | 气泡显示 | 气泡显示时机 | TTS 播报 | 数据库记录 |
|------|---------|------------|---------|-----------|
| 非舰长+有粉丝牌+首次打卡 | ✅ | enableVoice=false: 立即<br>enableVoice=true: TTS播放时 | ✅（受 enableVoice 控制） | ✅ |
| 非舰长+有粉丝牌+重复打卡 | ✅ | enableVoice=false: 立即<br>enableVoice=true: TTS播放时 | ✅（受 enableVoice 控制） | ✅ |
| 舰长+首次打卡 | ✅ | enableVoice=false: 立即<br>enableVoice=true: TTS播放时 | ✅ | ✅ |
| 舰长+重复打卡 | ✅ | enableVoice=false: 立即<br>enableVoice=true: TTS播放时 | ✅ | ✅ |

**说明**：
- 当 `enableVoice=true` 时，气泡通过 `g_checkinTTSPlayCallback` 在 TTS 播放开始时同步显示，确保语音和气泡同时出现
- 当 `enableVoice=false` 时，气泡通过 `g_aiReplyCallback` 在生成回复后立即显示

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
    bool hasMedal = false;      // 是否佩戴粉丝牌
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
弹幕接收 (guardLevel != 0 || hasMedal)
    ↓
IsCheckinMessage() 检测打卡
    ↓
RecordCheckin() 持久化打卡记录
    ↓
CalculateContinuousDays() 计算天数
    ↓
判断 guardLevel > 0？
    ├─ 是 → GenerateCheckinAnswerAsync() 生成 AI 回复 → TTS 播报
    └─ 否 → GetFallbackAnswer() 固定回复模板
    ↓
气泡显示 (g_aiReplyCallback)
```

**事件解耦**：`DanmuProcessor` 与 `CaptainCheckInModule` 通过 `CaptainDanmuHandler` 回调接口解耦，无直接头文件依赖。

> **详细设计**：见 `design.md` 第 620-678 行「事件解耦设计」章节。

**注意**：AI 回复的 TTS 播报**受 `enableVoice` 配置影响**。关闭 `enableVoice` 后不进行播报。

## AI Prompt 模板

```
用户{username}是一位舰长，连续第{continuousDays}天打卡。
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
