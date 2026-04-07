# Proposal: async-network-requests

## Why

项目要求所有网络请求必须使用异步实现。当前多个模块存在同步或协程网络请求：
- `MiniMaxAIChatProvider`、`DeepSeekAIChatProvider` 使用同步 WinHTTP API（`MakeSyncHttpsRequest`）
- `MiniMaxTTSProvider`、`XiaomiTTSProvider` 使用同步 WinHTTP API
- `MimoTTSClient`、`BliveManager` 使用协程 `Network::MakeHttpsRequest`

这些都会阻塞调用线程或依赖协程调度器，不符合纯异步架构要求。

## What Changes

- 新增 `Network::MakeHttpsRequestAsync` 纯异步 HTTP 接口（WinHTTP callback 模式，不依赖协程调度器）
- 新增 `Network::MakeWebSocketConnectionAsync` 纯异步 WebSocket 连接接口
- 新增 `Network::SendToWebsocketAsync` 纯异步 WebSocket 发送接口
- 改造所有 AI/TTS Provider（MiniMaxAIChatProvider、DeepSeekAIChatProvider、MiniMaxTTSProvider、XiaomiTTSProvider）使用异步接口
- 改造 `MimoTTSClient` 使用异步接口，删除协程等待逻辑
- 改造 `BliveManager` 全 callback 化，删除协程管理（networkCoroutines）
- 移除相关 spec 中的同步实现要求章节

## Capabilities

### New Capabilities

（无新增 capability）

### Modified Capabilities

- `ai-chat-provider`: 网络请求方式从同步改为异步
- `mimo-tts-integration`: 网络请求方式从协程改为异步
- `tts-provider`: 网络请求方式从同步改为异步

## Impact

- **代码变更**:
  - `MonsterOrderWilds/Network.h` — 新增异步接口声明
  - `MonsterOrderWilds/Network.cpp` — 新增异步接口实现
  - `MonsterOrderWilds/MiniMaxAIChatProvider.cpp` — 改用异步
  - `MonsterOrderWilds/DeepSeekAIChatProvider.cpp` — 改用异步
  - `MonsterOrderWilds/MiniMaxTTSProvider.cpp` — 改用异步
  - `MonsterOrderWilds/XiaomiTTSProvider.cpp` — 改用异步
  - `MonsterOrderWilds/MimoTTSClient.cpp` — 改用异步
  - `MonsterOrderWilds/BliveManager.h/.cpp` — 全 callback 化
- **文档变更**:
  - `openspec/specs/ai-chat-provider/spec.md`
  - `openspec/specs/mimo-tts-integration/spec.md`
  - `openspec/specs/tts-provider/spec.md`
- **调用方影响**: 无 — 接口语义不变（CallAPI 保持同步语义）
