## Why

当前舰长签到模块在处理高并发签到弹幕时存在严重阻塞问题：AI API 同步等待（1-3秒）和数据库 I/O 都在 `profilesLock_` 锁内执行，导致 UI 线程被阻塞，应用无响应。必须改为异步架构解除阻塞。

## What Changes

1. **AI 调用异步化**：将 `GenerateCheckinAnswerSync` 改为 `GenerateCheckinAnswerAsync`，AI API 调用移到后台线程，结果通过回调通知
2. **锁粒度优化**：`PushDanmuEvent` 只在访问共享数据（profiles）时短暂持锁，AI 调用在锁外执行，数据库操作通过 `SaveProfileAsync` 异步化（仍短暂持锁写入内存副本）
3. **数据库操作异步化**：数据库读写移到后台线程执行
4. **TTS 队列改进**：`asyncPendingQueue_` 支持多并发请求（上限3个），避免串行瓶颈

## Non-Goals

- 不改变现有的 AI Prompt 模板和回复生成逻辑
- 不改变现有的用户画像数据结构和 SQLite Schema
- 不改变 TTS 引擎选择逻辑（mimo/sapi/auto）
- 不改变配置字段（现有字段不变）

## Capabilities

### New Capabilities
- `captain-checkin-async`: 舰长签到异步处理能力
  - AI 回复异步生成，UI 线程不阻塞
  - 数据库操作异步执行，不持锁等待
  - 高并发场景下保证 UI 响应性

### Modified Capabilities
- `captain-checkin-ai-reply`: 修改 AI 调用和数据库操作的异步执行方式
  - 原：同步阻塞调用（`GenerateCheckinAnswerSync`）
  - 新：异步回调模式（`GenerateCheckinAnswerAsync`）

## Impact

**修改的文件**：
- `CaptainCheckInModule.cpp/h` - 异步化改造
- `MiniMaxAIChatProvider.cpp/h` - 添加异步 API 调用支持
- `DeepSeekAIChatProvider.cpp/h` - 添加异步 API 调用支持
- `TextToSpeech.cpp/h` - TTS 队列并发优化

**受影响的模块**：
- UI 线程不再因签到处理而阻塞
- 高流量场景下（每秒 100+ 弹幕）仍能保持响应
