# Captain Checkin Async Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 解决高并发签到场景下 UI 线程阻塞问题

**Architecture:** 
- AI 调用移至后台线程，结果通过回调返回
- 锁粒度优化：只在访问共享数据时短暂持锁
- 数据库操作异步化，不阻塞主流程
- TTS 队列支持多并发请求

**Tech Stack:** C++/CLI, Windows MCI, SQLite, std::thread

---

## 1. CaptainCheckInModule 异步改造

**Files:**
- Modify: `MonsterOrderWilds/CaptainCheckInModule.h`
- Modify: `MonsterOrderWilds/CaptainCheckInModule.cpp`

- [x] 1.1 修改 `GenerateCheckinAnswerAsync` 方法声明（现有方法，需修改为异步实现）

```cpp
// 现有声明，需修改为异步实现
void GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback);
```

- [x] 1.2 实现真正的异步 `GenerateCheckinAnswerAsync`，使用 `std::thread` 执行 AI 调用

```cpp
void CaptainCheckInModule::GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback) {
    CheckinEvent evtCopy = event;
    auto resultCb = [callback](const AnswerResult& result) {
        if (callback) callback(result);
    };
    
    std::thread([this, evtCopy, resultCb]() {
        AnswerResult result = GenerateCheckinAnswerSync(evtCopy, nullptr);
        resultCb(result);
    }).detach();
}
```
```

- [x] 1.3 重构 `PushDanmuEvent`，优化锁粒度

```cpp
void CaptainCheckInModule::PushDanmuEvent(const CaptainDanmuEvent& event) {
    // 学习流程：短暂持锁访问 profiles
    {
        lock_guard<std::mutex> lock(profilesLock_);
        LoadProfileFromDb(event.uid);
        auto it = profiles_.find(event.uid);
        // ... 学习逻辑（关键词提取、SaveProfileToDb 等）
    } // 锁在这里释放
    
    // 签到流程：锁外执行
    if (IsCheckinMessage(event.content)) {
        RecordCheckin(event.uid, event.username, event.sendDate);
        int32_t continuousDays = CalculateContinuousDays(event.uid, event.sendDate);
        
        CheckinEvent checkinEvt;
        checkinEvt.uid = event.uid;
        checkinEvt.username = event.username;
        checkinEvt.continuousDays = continuousDays;
        checkinEvt.checkinDate = event.sendDate;
        GenerateCheckinAnswerAsync(checkinEvt, [this, event](const AnswerResult& result) {
            // 回调处理 TTS 播放（使用 TTSManager::SpeakCheckinTTS）
            if (result.success && !result.answerContent.empty()) {
                TTSManager::Inst()->SpeakCheckinTTS(Utf8ToWstring(result.answerContent), event.username);
            }
        });
    }
}
```

- [x] 1.4 添加数据库异步操作辅助方法

```cpp
void SaveProfileAsync(const UserProfile& profile);
void LoadProfileAsync(uint64_t uid, std::function<void(UserProfile)> callback);
```

## 2. AI Chat Provider 异步支持

**Files:**
- Modify: `MonsterOrderWilds/MiniMaxAIChatProvider.h`
- Modify: `MonsterOrderWilds/MiniMaxAIChatProvider.cpp`
- Modify: `MonsterOrderWilds/DeepSeekAIChatProvider.h`
- Modify: `MonsterOrderWilds/DeepSeekAIChatProvider.cpp`

- [x] 2.1 新增异步 API 调用方法

```cpp
void CallAPIAsync(const std::string& prompt, std::function<void(bool, const std::string&)> callback);
```

- [x] 2.2 原有 `CallAPI` 方法保持同步，内部使用条件变量等待

## 3. TTS 队列并发优化

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h`
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

- [x] 3.1 修改 `asyncPendingQueue_` 支持多并发请求

```cpp
// 将 hasCurrentRequest_ 从 bool 改为计数器
std::atomic<int> activeRequestCount_;
static constexpr int MAX_CONCURRENT_TTS = 3;
```

- [x] 3.2 修改 `Tick()` 中的请求检查逻辑

```cpp
if (asyncPendingQueue_.empty() || activeRequestCount_ >= MAX_CONCURRENT_TTS) {
    return; // 队列空或达到并发上限
}
```

## 4. 编译验证

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`

- [x] 4.1 确保所有修改的文件添加到 vcxproj

- [x] 4.2 MSBuild 编译验证 (Debug: 0 errors, 0 warnings)

```bash
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "JonysandMHDanmuTools.sln" -p:Configuration=Debug -p:Platform=x64 -t:Build -m
```

## 5. 单元测试

**Files:**
- Modify: `MonsterOrderWilds/CaptainCheckInModuleTests.cpp`

- [x] 5.1 添加 `GenerateCheckinAnswerAsync` 单元测试

```cpp
#ifdef RUN_UNIT_TESTS
TEST_CASE("GenerateCheckinAnswerAsync returns result via callback") {
    CaptainCheckInModule& module = CaptainCheckInModule::Inst();
    CheckinEvent evt;
    evt.uid = 123456;
    evt.username = "test_user";
    evt.continuousDays = 1;
    evt.checkinDate = 20260412;
    
    bool callbackCalled = false;
    module.GenerateCheckinAnswerAsync(evt, [&callbackCalled](const AnswerResult& result) {
        callbackCalled = true;
        CHECK(result.success);
    });
    
    // 等待回调（异步）
    std::this_thread::sleep_for(std::chrono::seconds(5));
    CHECK(callbackCalled);
}
#endif
```

- [x] 5.2 运行单元测试验证 (需使用 UnitTest 配置编译运行)

```bash
# 运行 CaptainCheckInModuleTests 项目
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "MonsterOrderWilds\CaptainCheckInModuleTests.vcxproj" -p:Configuration=Debug -p:Platform=x64 -t:Build -m
# 预期：编译成功，输出包含 [PASS] 标记
```

## 调试说明

### DEBUG 模式下的 TEST_CAPTAIN_REPLY_LOCAL 宏

`framework.h` 中定义：
```cpp
#define TEST_CAPTAIN_REPLY_LOCAL _DEBUG
```

**作用**：DEBUG 模式下，跳过舰长等级检测（guardLevel >= 3），允许本地测试签到功能。

**注意事项**：
- `#ifndef TEST_CAPTAIN_REPLY_LOCAL` 块内在 DEBUG 模式下会被跳过
- 使用 `#if TEST_CAPTAIN_REPLY_LOCAL`（检查值）而不是 `#ifdef TEST_CAPTAIN_REPLY_LOCAL`（检查定义）来区分 DEBUG/RELEASE 行为
- `#ifdef` 只检查宏是否被定义过，即使值为 0 也会执行 `#ifdef` 分支

---

## Bug 修复记录 (2026-04-14)

### Bug 1: UI 卡顿问题

**症状**：TTS 请求时 UI 线程阻塞

**根因**：`MiniMaxTTSProvider::RequestTTS` 和 `XiaomiTTSProvider::RequestTTS` 调用 `MakeHttpsRequestAsync` 后使用 `cv.wait()` 阻塞等待，阻塞了 UI 线程

**修复**：
- `MiniMaxTTSProvider.cpp`：删除 `cv.wait()` 阻塞，改为真正异步回调
- `XiaomiTTSProvider.cpp`：同上

### Bug 2: 请求重复启动

**症状**：队列中同一请求被重复启动多次，导致后续请求永远得不到处理

**根因**：`ProcessAsyncTTS` 的第二个 while 循环使用 `asyncPendingQueue_.front()` 但没有实际处理该请求

**修复**：`TextToSpeech.cpp` 中添加状态检查，确保每个请求只被处理一次

### Bug 3: 播放时音频被中断

**症状**：正在播放的音频被后续请求打断

**根因**：`ProcessPlayingState` 中直接调用 `audioPlayer->Play()` 会中断正在播放的音频

**修复**：
- `TextToSpeech.cpp`：添加 `IsPlaying()` 检查，只有在播放完成时才启动新请求
- `AudioPlayer.cpp`：`IsPlaybackComplete()` 当 MCI 报告 "stopped" 时重置 `playing = false`

### Bug 4: temp 文件未删除

**症状**：播放完成后临时文件残留

**根因**：播放完成时没有调用 `CleanupTempFile()`

**修复**：`TextToSpeech.cpp` 播放完成时调用 `audioPlayer->CleanupTempFile()`

### Bug 5: 语音被截断

**症状**：较长的语音在 5 秒时被强制截断

**根因**：`ProcessPlayingState` 中硬编码 5 秒超时 `playbackElapsed > 5000`

**修复**：
- `TextToSpeech.h`：`PLAYBACK_TIMEOUT_SECONDS` 设为 0（不超时）
- `TextToSpeech.cpp`：只有当 MCI 报告播放完成时才结束

### Bug 6: playing 标志未重置

**症状**：第一个 TTS 播放完成后，后续 TTS 请求无法播放

**根因**：`IsPlaybackComplete()` 返回 true 时没有重置 `playing` 标志

**修复**：
- `AudioPlayer.h`：`playing` 改为 `mutable`
- `AudioPlayer.cpp`：`IsPlaybackComplete()` 中重置 `playing = false`

## 修改的文件汇总

| 文件 | 修改内容 |
|------|----------|
| `MiniMaxTTSProvider.cpp` | 删除 cv.wait() 阻塞，改为异步回调 |
| `XiaomiTTSProvider.cpp` | 删除 cv.wait() 阻塞，改为异步回调 |
| `AudioPlayer.cpp` | `IsPlaybackComplete()` 中重置 `playing`；播放时删除旧 temp 文件 |
| `AudioPlayer.h` | `playing` 改为 `mutable` |
| `TextToSpeech.cpp` | 添加 `IsPlaying()` 检查；添加 `playbackStarted` 标志；播放完成时清理 temp 文件；修复请求重复启动 bug |
| `TextToSpeech.h` | `PLAYBACK_TIMEOUT_SECONDS = 0`；`AsyncTTSRequest` 添加 `playbackStarted` 字段 |

