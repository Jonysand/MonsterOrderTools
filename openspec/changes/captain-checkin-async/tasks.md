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
- 使用 `#ifdef TEST_CAPTAIN_REPLY_LOCAL` 区分 DEBUG/RELEASE 行为
- 预编译指令中避免使用 `#if !TEST_CAPTAIN_REPLY_LOCAL`，应使用 `#ifdef`/`#ifndef`

