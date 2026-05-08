# TTS Playing 状态超时保护实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 SAPI 和非 SAPI 的 Playing 状态添加超时保护，防止并发槽被永久占用导致队列死锁。

**Architecture:** 在 `ProcessPlayingStateInternal` 中为 SAPI 分支添加独立的播放超时检查；将全局 `PLAYBACK_TIMEOUT_SECONDS` 从 0 改为 60 秒；为 `playbackStartTime` 在未设置时的行为添加保护；添加单元测试验证超时逻辑。

**Tech Stack:** C++17, MSBuild, 自定义单元测试框架（`TestLog` + `RUN_UNIT_TESTS` 宏）

---

## 文件结构

| 文件 | 责任 | 变更类型 |
|------|------|----------|
| `MonsterOrderWilds/TextToSpeech.h` | 常量定义（`PLAYBACK_TIMEOUT_SECONDS`、`SAPI_PLAYBACK_TIMEOUT_SECONDS`）和 `AsyncTTSRequest` 结构 | 修改 |
| `MonsterOrderWilds/TextToSpeech.cpp` | `ProcessPlayingStateInternal` SAPI 分支超时逻辑 | 修改 |
| `MonsterOrderWilds/TextToSpeechTests.cpp` | 新增超时相关单元测试 | 修改 |

---

## Task 1: 添加 SAPI 播放超时常量并修改非 SAPI 播放超时

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h:187`

- [ ] **Step 1: 修改播放超时常量**

将 `PLAYBACK_TIMEOUT_SECONDS` 从 `0` 改为 `60`，并添加 `SAPI_PLAYBACK_TIMEOUT_SECONDS`：

```cpp
	static constexpr int PLAYBACK_TIMEOUT_SECONDS = 60;		// 播放超时（秒），0表示不超时
	static constexpr int SAPI_PLAYBACK_TIMEOUT_SECONDS = 30;	// SAPI播放超时（秒），防止回调丢失导致卡死
```

**理由：**
- `PLAYBACK_TIMEOUT_SECONDS = 0` 表示永不超时，若 `audioPlayer->IsPlaybackComplete()` 状态异常，请求将永久占用并发槽
- 60 秒对于正常 TTS 播放足够（通常几秒到十几秒）
- SAPI 单独设置 30 秒，因为 SAPI 播放通常更快，且更依赖回调机制

---

## Task 2: 为 SAPI Playing 状态添加超时检查

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp:921-934`

- [ ] **Step 1: 在 ProcessPlayingStateInternal 的 SAPI 分支添加超时**

当前 SAPI 分支（第 926-933 行）：

```cpp
    // 处理 SAPI 请求完成
    if (req.engineType == TTSEngineType::SAPI) {
        if (req.sapiStreamEnded) {
            LOG_INFO(TEXT("TTS Async: SAPI playback completed (SPEI_STREAM_ENDED)"));
            req.state = AsyncTTSState::Completed;
            consecutiveFailures = 0;
            lastFailureTime = std::chrono::steady_clock::now();
        }
        return;
    }
```

修改为：

```cpp
    // 处理 SAPI 请求完成
    if (req.engineType == TTSEngineType::SAPI) {
        if (req.sapiStreamEnded) {
            LOG_INFO(TEXT("TTS Async: SAPI playback completed (SPEI_STREAM_ENDED)"));
            req.state = AsyncTTSState::Completed;
            consecutiveFailures = 0;
            lastFailureTime = std::chrono::steady_clock::now();
            return;
        }
        
        // SAPI 播放超时检查：防止回调丢失导致永久占用并发槽
        if (SAPI_PLAYBACK_TIMEOUT_SECONDS > 0 && req.playbackStartTime != std::chrono::steady_clock::time_point()) {
            auto now = std::chrono::steady_clock::now();
            auto playbackElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.playbackStartTime).count();
            if (playbackElapsed > SAPI_PLAYBACK_TIMEOUT_SECONDS) {
                LOG_WARNING(TEXT("TTS Async: SAPI playback timeout (%lld seconds), treating as completed"), (long long)playbackElapsed);
                req.state = AsyncTTSState::Completed;
                consecutiveFailures = 0;
                lastFailureTime = std::chrono::steady_clock::now();
            }
        }
        return;
    }
```

**关键检查：**
- `playbackStartTime != time_point()`：确保已经开始播放（`SpeakWithSapiAsync` 应在开始播放时设置此字段）
- 超时后标记为 `Completed` 而非 `Failed`：因为音频可能实际已经播放完毕，只是回调丢失

---

## Task 3: 验证 playbackStartTime 在 SAPI 请求中被正确设置

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`（搜索 `SpeakWithSapiAsync` 或 SAPI 请求启动位置）

- [ ] **Step 1: 检查 SAPI 请求的 playbackStartTime 设置**

确保在 SAPI 请求开始播放时设置了 `playbackStartTime`。搜索代码中启动 SAPI 播放的位置（可能在 `ProcessPendingRequestInternal` 或类似位置），确认有：

```cpp
req.playbackStartTime = std::chrono::steady_clock::now();
```

如果未设置，需要添加。当前非 SAPI 请求在 `ProcessPlayingStateInternal` 第 954 行有：

```cpp
req.playbackStartTime = std::chrono::steady_clock::now();  // 重置播放超时计时
```

需要确认 SAPI 请求是否也有类似设置。如果在 `SpeakWithSapiAsync` 或 `SpeakWithSapi` 中没有设置，需要在发起 SAPI  speak 调用后添加：

```cpp
// 在调用 pVoice->Speak() 或类似启动播放后
req.playbackStartTime = std::chrono::steady_clock::now();
req.playbackStarted = true;
```

---

## Task 4: 编译验证

- [ ] **Step 1: 编译 Release x64**

Run:
```powershell
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: `0 个错误`

---

## Task 5: 添加单元测试

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeechTests.cpp`

- [ ] **Step 1: 添加 SAPI 播放超时测试**

在 `TextToSpeechTests.cpp` 末尾、`RunTextToSpeechTests()` 之前添加：

```cpp
void TestSapiPlaybackTimeoutConstant() {
    // 验证 SAPI 播放超时常量设置合理
    if (SAPI_PLAYBACK_TIMEOUT_SECONDS > 0) {
        TestLog("[PASS] TestSapiPlaybackTimeoutConstant - SAPI_PLAYBACK_TIMEOUT_SECONDS (%d) > 0", SAPI_PLAYBACK_TIMEOUT_SECONDS);
    } else {
        TestLog("[FAIL] TestSapiPlaybackTimeoutConstant - SAPI_PLAYBACK_TIMEOUT_SECONDS should be > 0");
    }
    
    // SAPI 超时应该小于等于普通播放超时
    if (SAPI_PLAYBACK_TIMEOUT_SECONDS <= PLAYBACK_TIMEOUT_SECONDS) {
        TestLog("[PASS] TestSapiPlaybackTimeoutConstant - SAPI_PLAYBACK_TIMEOUT_SECONDS (%d) <= PLAYBACK_TIMEOUT_SECONDS (%d)", 
            SAPI_PLAYBACK_TIMEOUT_SECONDS, PLAYBACK_TIMEOUT_SECONDS);
    } else {
        TestLog("[FAIL] TestSapiPlaybackTimeoutConstant - SAPI timeout should not exceed general playback timeout");
    }
}

void TestPlaybackTimeoutConstant() {
    // 验证播放超时常量已设置为合理值（不再是 0）
    if (PLAYBACK_TIMEOUT_SECONDS > 0) {
        TestLog("[PASS] TestPlaybackTimeoutConstant - PLAYBACK_TIMEOUT_SECONDS (%d) > 0", PLAYBACK_TIMEOUT_SECONDS);
    } else {
        TestLog("[FAIL] TestPlaybackTimeoutConstant - PLAYBACK_TIMEOUT_SECONDS should be > 0 to prevent deadlock");
    }
    
    // 验证播放超时不超过 5 分钟（避免过长等待）
    if (PLAYBACK_TIMEOUT_SECONDS <= 300) {
        TestLog("[PASS] TestPlaybackTimeoutConstant - PLAYBACK_TIMEOUT_SECONDS (%d) <= 300 seconds", PLAYBACK_TIMEOUT_SECONDS);
    } else {
        TestLog("[FAIL] TestPlaybackTimeoutConstant - PLAYBACK_TIMEOUT_SECONDS should be <= 300");
    }
}

void TestAsyncTTSRequestPlaybackTimeoutLogic() {
    // 验证播放超时逻辑：playbackStartTime 未设置时不应触发超时
    AsyncTTSRequest req;
    req.state = AsyncTTSState::Playing;
    req.engineType = TTSEngineType::SAPI;
    req.playbackStartTime = std::chrono::steady_clock::time_point(); // 未设置
    req.sapiStreamEnded = false;
    
    // 模拟 ProcessPlayingStateInternal 的超时检查逻辑
    bool wouldTimeout = false;
    if (SAPI_PLAYBACK_TIMEOUT_SECONDS > 0 && req.playbackStartTime != std::chrono::steady_clock::time_point()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.playbackStartTime).count();
        wouldTimeout = (elapsed > SAPI_PLAYBACK_TIMEOUT_SECONDS);
    }
    
    if (!wouldTimeout) {
        TestLog("[PASS] TestAsyncTTSRequestPlaybackTimeoutLogic - No timeout when playbackStartTime not set");
    } else {
        TestLog("[FAIL] TestAsyncTTSRequestPlaybackTimeoutLogic - Should not timeout when playbackStartTime not set");
    }
    
    // 验证 playbackStartTime 设置后，超时能正确触发
    req.playbackStartTime = std::chrono::steady_clock::now() - std::chrono::seconds(SAPI_PLAYBACK_TIMEOUT_SECONDS + 1);
    wouldTimeout = false;
    if (SAPI_PLAYBACK_TIMEOUT_SECONDS > 0 && req.playbackStartTime != std::chrono::steady_clock::time_point()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.playbackStartTime).count();
        wouldTimeout = (elapsed > SAPI_PLAYBACK_TIMEOUT_SECONDS);
    }
    
    if (wouldTimeout) {
        TestLog("[PASS] TestAsyncTTSRequestPlaybackTimeoutLogic - Timeout correctly triggers after duration");
    } else {
        TestLog("[FAIL] TestAsyncTTSRequestPlaybackTimeoutLogic - Should timeout when elapsed > limit");
    }
}
```

- [ ] **Step 2: 在 RunTextToSpeechTests 中注册新测试**

修改 `RunTextToSpeechTests`：

```cpp
void RunTextToSpeechTests() {
    TestLog("=== TextToSpeech Tests ===");
    TestDynamicComboConstants();
    TestAsyncTTSRequestResetTiming();
    TestMaxTotalTimeoutConstant();
    TestRetryCountLimit();
    TestSapiSsmlEscaping();
    TestAsyncTTSRequestTotalStartTimePersistence();
    TestInstanceAliveFlag();
    TestSapiPlaybackTimeoutConstant();
    TestPlaybackTimeoutConstant();
    TestAsyncTTSRequestPlaybackTimeoutLogic();
    TestLog("=== TextToSpeech Tests Complete ===");
}
```

---

## Task 6: 运行单元测试并编译

- [ ] **Step 1: 编译 Debug 配置（启用 RUN_UNIT_TESTS）**

确认 `MonsterOrderWilds.vcxproj` 的 Debug 配置定义了 `RUN_UNIT_TESTS` 宏，然后编译：

```powershell
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"
```

- [ ] **Step 2: 运行单元测试**

运行生成的测试可执行文件（具体路径取决于项目配置）：

```powershell
.\x64\Debug\MonsterOrderWilds.exe
```

Expected: 测试输出包含 `[PASS]` 标记，无 `[FAIL]`

- [ ] **Step 3: 编译 Release 配置验证无回归**

```powershell
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: `0 个错误`

---

## Self-Review

**1. Spec coverage:**
- ✅ SAPI Playing 状态超时保护
- ✅ 非 SAPI 播放超时从 0 改为合理值
- ✅ playbackStartTime 边界条件保护
- ✅ 单元测试覆盖

**2. Placeholder scan:**
- 无 "TBD"/"TODO"/"implement later"
- 所有代码块包含完整可编译代码
- 无模糊描述

**3. Type consistency：**
- `SAPI_PLAYBACK_TIMEOUT_SECONDS` 类型为 `int`，与其他 `*_SECONDS` 常量一致
- `playbackElapsed` 类型为 `long long`（与现有代码模式一致）
- 使用 `std::chrono::steady_clock`（与现有代码一致）

---

## 实现后检查清单

- [ ] `PLAYBACK_TIMEOUT_SECONDS` 不再是 0
- [ ] `SAPI_PLAYBACK_TIMEOUT_SECONDS` 已定义且大于 0
- [ ] SAPI 分支在 `sapiStreamEnded` 为 false 时检查超时
- [ ] 超时检查包含 `playbackStartTime != time_point()` 保护
- [ ] 超时后状态变为 `Completed`（不是 `Failed`）
- [ ] 新测试在 `RunTextToSpeechTests()` 中被调用
- [ ] Release 编译 0 错误
- [ ] Debug 编译并运行测试通过
