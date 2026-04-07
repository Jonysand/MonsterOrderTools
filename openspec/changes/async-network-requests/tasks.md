## 1. Network 层新增异步接口

- [x] 1.1 在 `Network.h` 中新增 `MakeHttpsRequestAsync` 声明（WinHTTP callback 模式）
- [x] 1.2 在 `Network.cpp` 中实现 `MakeHttpsRequestAsync`（使用 `WinHttpSetStatusCallback` 注册异步回调）
- [x] 1.3 在 `Network.h` 中新增 `MakeWebSocketConnectionAsync` 声明
- [x] 1.4 在 `Network.cpp` 中实现 `MakeWebSocketConnectionAsync`
- [x] 1.5 在 `Network.h` 中新增 `SendToWebsocketAsync` 声明
- [x] 1.6 在 `Network.cpp` 中实现 `SendToWebsocketAsync`

## 2. AI Provider 异步改造

- [x] 2.1 `MiniMaxAIChatProvider` 改用 `Network::MakeHttpsRequestAsync`
- [x] 2.2 `DeepSeekAIChatProvider` 改用 `Network::MakeHttpsRequestAsync`
- [x] 2.3 删除 `MiniMaxAIChatProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法
- [x] 2.4 删除 `DeepSeekAIChatProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法

## 3. TTS Provider 异步改造

- [x] 3.1 `MiniMaxTTSProvider` 改用 `Network::MakeHttpsRequestAsync`
- [x] 3.2 `XiaomiTTSProvider` 改用 `Network::MakeHttpsRequestAsync`
- [x] 3.3 删除 `MiniMaxTTSProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法
- [x] 3.4 删除 `XiaomiTTSProvider.cpp` 中的 `MakeSyncHttpsRequest` 方法

## 4. MimoTTSClient 异步改造

- [x] 4.1 `MimoTTSClient` 改用 `Network::MakeHttpsRequestAsync`
- [x] 4.2 删除协程等待逻辑（`while (coroutine.resume()) { Sleep(10); }`）

## 5. BliveManager 全 callback 化

- [x] 5.1 `BliveManager.h` 新增 `AsyncRequest` 结构替换 `Network::NetworkCoroutine`
- [x] 5.2 `BliveManager.h` 将 `networkCoroutines` 改为 `networkRequests`
- [x] 5.3 `BliveManager::Tick()` 改造为处理 callback
- [x] 5.4 `BliveManager::Start()` 改用 `MakeHttpsRequestAsync`
- [x] 5.5 `BliveManager::End()` 改用 `MakeHttpsRequestAsync`
- [x] 5.6 `BliveManager::StartAppHeartBeat()` 改用 `MakeHttpsRequestAsync`
- [x] 5.7 `BliveManager` WebSocket 部分改用 `MakeWebSocketConnectionAsync` 和 `SendToWebsocketAsync`
- [x] 5.8 删除 `BliveManager.cpp` 中的协程等待逻辑

## 6. Spec 文档同步更新

- [x] 6.1 更新 `openspec/specs/ai-chat-provider/spec.md`，移除"同步请求实现要求"章节
- [x] 6.2 更新 `openspec/specs/mimo-tts-integration/spec.md`（协程改为 callback）
- [x] 6.3 更新 `openspec/specs/tts-provider/spec.md`（无需修改，已使用 callback 模式）

## 7. 验证

- [x] 7.1 MSBuild 编译验证（Release x64）
- [x] 7.2 单元测试验证（Debug 配置，RUN_UNIT_TESTS）

## 8. 冗余代码清理（Phase 6）

**前置条件**：Phase 5 验证通过（编译成功 + 单元测试通过）

- [x] 8.1 删除 `Network.h` 中的 `NetworkCoroutine` 结构体
- [x] 8.2 删除 `Network.cpp` 中的协程版本 `MakeHttpsRequest`
- [x] 8.3 删除 `Network.cpp` 中的协程版本 `MakeWebSocketConnection`
- [x] 8.4 删除 `Network.cpp` 中的协程版本 `SendToWebsocket`
- [x] 8.5 删除 `Network.h` 中的 `HttpsAwaiter` 结构体和 `HttpsCorouineUtils` namespace
- [x] 8.6 删除 `BliveManager.h` 中的 `networkCoroutines` 成员
- [x] 8.7 删除 `BliveManager.cpp` 中 `Tick()` 里的协程 resume 循环

**验收标准**：
- [x] MSBuild 编译成功（Release x64）
- [x] 单元测试全部通过
- [x] 运行时行为与 Phase 5 验证时一致

## 9. 代码审查修复（三轮审查）

**审查轮次**：2026-04-07

### Critical Bug 修复

- [x] 9.1 修复 `Network.cpp` ASYNC_RESULT 错误码提取错误（添加 sizeof 检查和正确成员访问）
- [x] 9.2 修复 `HttpsAsyncContext` 双重 delete 问题（添加 `cleanupDone` 原子标志）
- [x] 9.3 修复 WebSocket 句柄双重关闭问题（移除 StartWebSocketReceive 中的 CloseHandle）
- [x] 9.4 修复双重线程 spawn 优化（删除多余的 std::thread spawn）

### High Bug 修复

- [x] 9.5 修复所有 async callback 缺少异常安全问题（添加 try-catch）
- [x] 9.6 修复 WebSocket 消息体截断问题（添加 bytesTransferred 完整性检查）

### 死代码清理

- [x] 9.7 移除 `AsyncRequest` 中未使用的 `WEBSOCKET_CONNECT`/`WEBSOCKET_SEND` 类型和 callback 成员

**验收标准**：
- [x] MSBuild 编译成功（Release x64）
- [x] 代码审查全部 Critical/High 问题已修复

## 10. UAF 预防修复（第四轮审查）

**问题**：`StartWebSocketReceive` 启动 detached 线程调用 `BliveManager::Inst()->IsConnected()`，当 `Destroy()` 被调用时 `__Instance` 置为 `nullptr`，但 detached 线程可能仍在访问已销毁对象。

**修复方案**：
- [x] 10.1 修改 `DECLARE_SINGLETON`/`DEFINE_SINGLETON` 宏，新增 `GetDestroyingFlag()` 方法
- [x] 10.2 修改 `Destroy()` 在删除实例前设置 `__Destroying` 标志
- [x] 10.3 修改 `BliveManager::IsConnected()` 检查 `GetDestroyingFlag()`
- [x] 10.4 删除 `BliveManager.cpp` 中过时的 `destroying_` 静态成员定义

**验收标准**：
- [x] MSBuild 编译成功（Release x64）
- [x] `BliveManager.cpp` 不再定义独立的 `destroying_` 静态成员

## 11. 补充审查修复（第五轮审查）

**审查轮次**：2026-04-07（补充）

### 问题修复

- [x] 11.1 修复 `EventSystem.h` 中 `Invoke` 方法缺少异常保护问题（添加 try-catch）
- [x] 11.2 修复 `TextToSpeech.cpp` 中 `ProcessPlayingState` 缺少线程同步问题（添加 `asyncMutex_` 锁保护）

**验收标准**：
- [x] MSBuild 编译成功（Release x64）
- [x] 所有 Event handler 调用有异常保护
- [x] TTS 状态机访问 `req` 对象时有正确的线程同步

## 12. 审查总结

**审查轮次**：共 5 轮（3 轮主审查 + 2 轮补充）

**发现并修复的问题**：
- Critical: 4 个（ASYNC_RESULT 错误码、双重 delete、WebSocket 句柄双重关闭、UAF）
- High: 6 个（callback 异常安全 x4、AsyncRequest 死字段 x1、额外 callback 遗漏 x1）
- Medium: 2 个（EventSystem 异常保护、TextToSpeech 线程同步）

**遗留问题**（不在本次变更范围）：
- Provider `cv.wait` 无超时机制（设计决策，暂不修改）
- EventSystem 整体异常恢复机制（EventSystem 模块独立演进）
- TextToSpeech `asyncPendingQueue_` 竞态（通过锁保护缓解）

**验收标准**：
- [x] MSBuild 编译成功（Release x64）
- [x] 所有 Critical/High 问题已修复
- [x] 代码质量符合项目标准
