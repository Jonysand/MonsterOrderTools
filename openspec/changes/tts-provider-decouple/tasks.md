# TTS Provider Decouple Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** TTSManager 通过 `std::unique_ptr<ITTSProvider>` 接口持有 TTS 提供者，消除直接依赖 MimoTTSClient，清理 AsyncTTSRequest 残留字段。

**Architecture:** TTSManager 通过依赖注入持有 `std::unique_ptr<ITTSProvider>`，通过接口调用 `RequestTTS()`。MiMo 特有逻辑封装在 XiaomiTTSProvider 内部，MimoTTSClient 作为其内部实现。

**Tech Stack:** C++ (C++/CLI), WPF, ITTSProvider interface

---

## 1. 清理 AsyncTTSRequest 残留字段

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h:25-40`

- [ ] **Step 1: 删除 dialect 和 role 字段**

```cpp
struct AsyncTTSRequest
{
    TString text;
    AsyncTTSState state = AsyncTTSState::Pending;
    std::vector<uint8_t> audioData;
    std::string responseFormat;
    std::chrono::steady_clock::time_point startTime;
    int retryCount = 0;
    std::string errorMessage;
    bool isCheckinTTS = false;
    std::string checkinUsername;
    std::string voice;
    int speed = 0;
};
```

---

## 2. 重构 TTSManager 持有方式

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h:135-137`
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

- [ ] **Step 1: 将 `MimoTTSClient*` 替换为 `std::unique_ptr<ITTSProvider>`**

TextToSpeech.h 修改：
```cpp
#if USE_MIMO_TTS
    // TTS 提供者（通过接口持有）
    std::unique_ptr<ITTSProvider> ttsProvider;
    // 音频播放器
    AudioPlayer* audioPlayer{ NULL };
```

- [ ] **Step 2: 修改构造函数初始化**

TextToSpeech.cpp 构造函数中：
```cpp
// 删除: mimoClient = new MimoTTSClient();
// 添加: ttsProvider = TTSProviderFactory::Create(credentialJson);
```

- [ ] **Step 3: 修改析构函数**

```cpp
// 删除: if (mimoClient) { delete mimoClient; mimoClient = nullptr; }
// unique_ptr 自动清理，无需手动 delete
```

- [ ] **Step 4: 修改 ProcessPendingRequest 使用接口调用**

在 `ProcessPendingRequest` 中：
```cpp
// 删除: mimoClient->RequestTTS(ttsReq, [this, it](...) { ... });
// 改为: ttsProvider->RequestTTS(ttsReq, [this, it](...) { ... });
```

---

## 3. 验证编译

**Files:**
- Build: `JonysandMHDanmuTools.sln`

- [ ] **Step 1: MSBuild 编译**

Run: `MSBuild.exe JonysandMHDanmuTools.sln -p:Configuration=Release -p:Platform=x64 -t:Build -m`

Expected: `0 个错误`

---

## 4. 单元测试验证

**Files:**
- Test: `MonsterOrderWilds/ConfigManagerTests.cpp`
- Test: `MonsterOrderWilds/TTSProviderTests.cpp`

- [ ] **Step 1: 运行 ConfigManagerTests**

- [ ] **Step 2: 运行 TTSProviderTests**

---

## Task 来源

| Scenario | Task |
|----------|------|
| 请求结构体清理 | Task 1: 清理 AsyncTTSRequest 残留字段 |
| 通过接口调用 TTS | Task 2: 重构 TTSManager 持有方式 |
| 接口所有特化字段 | Task 2: 重构 TTSManager 持有方式 |
| 异步请求处理 | Task 2: 重构 TTSManager 持有方式 |
| 验证编译 | Task 3: 验证编译 |
| 单元测试覆盖 | Task 4: 单元测试验证 |
