## 1. Network 层新增异步接口

- [ ] 1.1 在 `Network.h` 中新增 `MakeHttpsRequestAsync` 声明（WinHTTP callback 模式）
- [ ] 1.2 在 `Network.cpp` 中实现 `MakeHttpsRequestAsync`（使用 `WinHttpSetStatusCallback` 注册异步回调）
- [ ] 1.3 在 `Network.h` 中新增 `MakeWebSocketConnectionAsync` 声明
- [ ] 1.4 在 `Network.cpp` 中实现 `MakeWebSocketConnectionAsync`
- [ ] 1.5 在 `Network.h` 中新增 `SendToWebsocketAsync` 声明
- [ ] 1.6 在 `Network.cpp` 中实现 `SendToWebsocketAsync`

## 2. AI Provider 异步改造

- [ ] 2.1 `MiniMaxAIChatProvider` 改用 `Network::MakeHttpsRequestAsync`
- [ ] 2.2 `DeepSeekAIChatProvider` 改用 `Network::MakeHttpsRequestAsync`
- [ ] 2.3 删除 `MiniMaxAIChatProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法
- [ ] 2.4 删除 `DeepSeekAIChatProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法

## 3. TTS Provider 异步改造

- [ ] 3.1 `MiniMaxTTSProvider` 改用 `Network::MakeHttpsRequestAsync`
- [ ] 3.2 `XiaomiTTSProvider` 改用 `Network::MakeHttpsRequestAsync`
- [ ] 3.3 删除 `MiniMaxTTSProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法
- [ ] 3.4 删除 `XiaomiTTSProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法

## 4. MimoTTSClient 异步改造

- [ ] 4.1 `MimoTTSClient` 改用 `Network::MakeHttpsRequestAsync`
- [ ] 4.2 删除协程等待逻辑（`while (coroutine.resume()) { Sleep(10); }`）

## 5. BliveManager 全 callback 化

- [ ] 5.1 `BliveManager.h` 新增 `AsyncRequest` 结构替换 `Network::NetworkCoroutine`
- [ ] 5.2 `BliveManager.h` 将 `networkCoroutines` 改为 `networkRequests`
- [ ] 5.3 `BliveManager::Tick()` 改造为处理 callback
- [ ] 5.4 `BliveManager::Start()` 改用 `MakeHttpsRequestAsync`
- [ ] 5.5 `BliveManager::End()` 改用 `MakeHttpsRequestAsync`
- [ ] 5.6 `BliveManager::StartAppHeartBeat()` 改用 `MakeHttpsRequestAsync`
- [ ] 5.7 `BliveManager` WebSocket 部分改用 `MakeWebSocketConnectionAsync` 和 `SendToWebsocketAsync`
- [ ] 5.8 删除 `BliveManager.cpp` 中的协程等待逻辑

## 6. Spec 文档同步更新

- [ ] 6.1 更新 `openspec/specs/ai-chat-provider/spec.md`，移除"同步请求实现要求"章节
- [ ] 6.2 更新 `openspec/specs/mimo-tts-integration/spec.md`（如有必要）
- [ ] 6.3 更新 `openspec/specs/tts-provider/spec.md`（如有必要）

## 7. 验证

- [ ] 7.1 MSBuild 编译验证（Release x64）
- [ ] 7.2 单元测试验证（Debug 配置，RUN_UNIT_TESTS）

## 8. 冗余代码清理（Phase 6）

**前置条件**：Phase 5 验证通过（编译成功 + 单元测试通过）

- [ ] 8.1 删除 `Network.h` 中的 `NetworkCoroutine` 结构体
- [ ] 8.2 删除 `Network.cpp` 中的协程版本 `MakeHttpsRequest`
- [ ] 8.3 删除 `Network.cpp` 中的协程版本 `MakeWebSocketConnection`
- [ ] 8.4 删除 `Network.cpp` 中的协程版本 `SendToWebsocket`
- [ ] 8.5 删除 `Network.h` 中的 `HttpsAwaiter` 结构体
- [ ] 8.6 删除 `BliveManager.h` 中的 `networkCoroutines` 成员
- [ ] 8.7 删除 `BliveManager.cpp` 中 `Tick()` 里的协程 resume 循环

**验收标准**：
- MSBuild 编译成功（Release x64）
- 单元测试全部通过
- 运行时行为与 Phase 5 验证时一致
