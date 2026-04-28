# TTS Playback Optimization

## 概述

本任务实现了 TTS 播报的队列遍历优化，修复了因只检查队首导致的播放间隔过长问题，并提高了 HTTP 并发上限。

## 优化内容

### 1. 队列遍历优化

**问题**：第二个 while 循环只检查队首，如果队首不是 Pending 就 break，导致后续 Pending 请求被阻塞。

**修复**：遍历整个队列找第一个 Pending 请求。

### 2. 提高并发上限

**修改**：`MAX_CONCURRENT_TTS` 从 3 提升到 64。

---

## 实施步骤

### Step 1: 优化队列遍历逻辑

**文件:** `MonsterOrderWilds/TextToSpeech.cpp`

修改 `ProcessAsyncTTS` 中的第二个 while 循环：

```cpp
while (activeRequestCount_ < MAX_CONCURRENT_TTS && !asyncPendingQueue_.empty()) {
    bool foundPending = false;
    for (auto it = asyncPendingQueue_.begin(); it != asyncPendingQueue_.end(); ++it) {
        if (it->state == AsyncTTSState::Pending) {
            it->startTime = std::chrono::steady_clock::now();
            activeRequestCount_++;
            LOG_INFO(TEXT("TTS Async: Starting new request for: %s (active: %d)"),
                it->text.c_str(), activeRequestCount_.load());
            ProcessPendingRequest(it);
            foundPending = true;
            break;
        }
    }
    if (!foundPending) {
        break;
    }
}
```

### Step 2: 提高并发上限

**文件:** `MonsterOrderWilds/TextToSpeech.h`

```cpp
static constexpr int MAX_CONCURRENT_TTS = 64;      // API并发请求数（从3提升到64）
```

---

## 文件变更总结

| 文件 | 变更 |
|------|------|
| `TextToSpeech.cpp` | 队列遍历优化（约15行） |
| `TextToSpeech.h` | `MAX_CONCURRENT_TTS` 从 3 改为 64 |

**总计: 2 个文件**

## 验证清单

| 检查项 | 状态 |
|--------|------|
| MSBuild 编译 | ✅ 0 个错误 |
| 队列遍历优化生效 | ✅ |
| 并发上限 64 | ✅ |
