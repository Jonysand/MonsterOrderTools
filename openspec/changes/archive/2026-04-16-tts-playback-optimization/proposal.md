## Why

当前 TTS 播报流程存在延迟问题：HTTP 响应完成后需要等待下一个 Tick() 才能开始播放，且播放完成后也需要等待下一个 Tick() 才能检查下一条。优化方案是引入"待播放队列"，HTTP 响应完成后立即进入队列等待播放，Tick() 检查队列并串行播放。

## What Changes

1. **新增待播放队列**：引入 `readyToPlayQueue_`，已完成 HTTP 请求和缓存的请求进入此队列
2. **新增缓存异步化**：TTS 响应数据通过异步线程写入缓存，不阻塞播放流程
3. **优化播放触发时机**：Tick() 检查待播放队列，当前一条播放完成后立即从队列取下一条播放
4. **状态机新增状态**：新增 `Caching`（缓存中）和 `Ready`（等待播放）状态
5. **移除 NormalMsgQueue 依赖**：弹幕 TTS 播报不再经过 `NormalMsgQueue`，直接发起 HTTP 请求

## Capabilities

### New Capabilities
- `async-tts-queue`: 异步 TTS 播放队列管理
  - HTTP 请求完成后进入待播放队列
  - 缓存写入在独立线程完成
  - 播放队列串行取出播放

### Modified Capabilities
- `mimo-tts-integration`: TTS 集成
  - 原实现：HTTP 响应完成后等待 Tick() 才播放
  - 新实现：HTTP 响应完成后进入待播放队列，Tick() 驱动播放

## Impact

**修改的文件**：
- `MonsterOrderWilds/TextToSpeech.h`: 新增状态、队列、接口
- `MonsterOrderWilds/TextToSpeech.cpp`: 重构播放逻辑
- `MonsterOrderWilds/TTSCacheManager.h`: 新增异步缓存接口
- `MonsterOrderWilds/TTSCacheManager.cpp`: 实现异步缓存

**性能影响**：
- HTTP 请求：保持最多 3 个并发
- 缓存写入：独立线程池，不阻塞主线程
- 播放延迟：消除等待 Tick() 的延迟

**行为变化**：
- 播报顺序不再严格按弹幕接收顺序，而是按 HTTP 响应完成顺序
- 播放更加及时，不会有 Tick() 周期的额外延迟
