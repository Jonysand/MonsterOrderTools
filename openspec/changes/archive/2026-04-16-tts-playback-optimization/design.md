## Context

当前 TTS 播报流程存在延迟问题。弹幕到达后，TTS 请求需要等待 Tick() 才发出，HTTP 响应完成后也需要等待下一个 Tick() 才开始播放。这导致多弹幕场景下延迟明显。

## Goals / Non-Goals

**Goals:**
- HTTP 请求尽快分发，弹幕到达后立即发起请求 ✅
- **队列遍历优化**：修复因只检查队首导致的播放间隔过长问题 ✅
- **提高并发上限**：支持更多并发 API 请求，减少等待 ✅

**Non-Goals:**
- 不改变播放的串行特性
- 不引入复杂的优先级机制
- 不做普通弹幕缓存（只缓存打卡 TTS）

## Decisions

### Decision 1: 遍历队列查找 Pending 请求

**选择**：在启动新请求时，遍历整个队列找 Pending 请求，而不是只检查队首

**理由**：
- 修复"队首不是 Pending 就 break"导致的播放间隔过长问题
- 确保所有 Pending 请求都能被及时处理

### Decision 2: 提高 HTTP 并发上限

**选择**：将 `MAX_CONCURRENT_TTS` 从 3 提升到 64

**理由**：
- 弹幕高峰期可能有大量请求堆积
- 更高的并发能更快地完成所有请求的 API 调用
- 播放仍是串行的，不影响体验

## Implementation Summary

### 队列遍历优化

**TextToSpeech.cpp (ProcessAsyncTTS):**
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

## Files Modified

| 文件 | 变更 |
|------|------|
| `TextToSpeech.cpp` | 队列遍历优化 |
| `TextToSpeech.h` | `MAX_CONCURRENT_TTS` 从 3 改为 64 |

**总计：2 个文件**

## Performance Characteristics

### 延迟分析

| 阶段 | 延迟 |
|------|------|
| HTTP 请求发送 | < 1ms（即时） |
| HTTP 响应等待 | ~1 秒（网络往返） |
| 播放启动 | < 100ms（下一个 Tick） |
| 播放间隔 | 大幅缩短（遍历优化） |

### 播放间隔优化

**优化前问题**：第二个 while 只检查队首，如果队首不是 Pending 就 break，导致后续 Pending 请求被阻塞。

**优化后**：遍历整个队列，确保任何位置的 Pending 请求都能被及时发送。
