## Context

项目要求所有网络请求必须使用异步实现。当前多个模块存在问题：
- `MiniMaxAIChatProvider`、`DeepSeekAIChatProvider` 使用同步 WinHTTP API（阻塞调用线程）
- `MiniMaxTTSProvider`、`XiaomiTTSProvider` 使用同步 WinHTTP API
- `MimoTTSClient`、`BliveManager` 使用协程 `Network::MakeHttpsRequest`

用户明确要求**不使用协程**，全部改为**纯 Callback 驱动的异步接口**。

## Goals / Non-Goals

**Goals:**
- 所有网络请求模块改为异步实现（HTTP + WebSocket）
- 不依赖协程调度器，使用 WinHTTP 原生异步 API
- 对外接口语义不变（CallAPI 保持同步语义，内部异步等待）
- 同步更新 spec 文档

**Non-Goals:**
- 不引入第三方 HTTP 库（继续使用 WinHTTP）
- 不修改 `CaptainCheckInModule` 等调用方（接口不变）

## Decisions

### Decision 1: 新增 `Network::MakeHttpsRequestAsync` 纯异步 HTTP 接口

**选择**：新增独立的异步方法，后续清理协程版本

**理由**：
- 阶段化实施：先新增异步接口，验证通过后再清理协程代码
- 避免破坏现有代码（协程版本保留可编译）
- 两种实现互不干扰
- HttpsAwaiter 为过渡期参考实现，验证完成后随协程版本一同清理

### Decision 2: Provider 内部通过 `condition_variable` 等待异步完成

**选择**：在 `CallAPI` 内部启动异步请求，然后等待

**实现模式**：
```cpp
bool CallAPI(const std::string& prompt, std::string& outResponse) {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    std::string response;
    
    MakeHttpsRequestAsync(host, port, path, headers, body,
        [&](bool success, const std::string& resp) {
            if (success) response = resp;
            {
                std::lock_guard<std::mutex> lock(mtx);
                done = true;
            }
            cv.notify_one();
        });
    
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&done]() { return done; });
    // 处理 response...
}
```

### Decision 3: BliveManager 全 callback 化

**选择**：用 `AsyncRequest` 结构替换 `Network::NetworkCoroutine`，Tick() 驱动 callback

**变更**：
```cpp
// Before
std::list<Network::NetworkCoroutine> networkCoroutines;
void Tick() { for (auto& coro : networkCoroutines) coro.resume(); }

// After
struct AsyncRequest {
    enum Type { HTTP, WEBSOCKET_CONNECT, WEBSOCKET_SEND };
    Type type;
    HttpsAsyncCallback httpCallback;        // for HTTP
    WebSocketAsyncCallback wsConnectCallback;  // for WebSocket connect
    std::function<void(bool, DWORD)> wsSendCallback;  // for WebSocket send
    bool completed = false;
    std::string response;
    DWORD error = 0;
    HINTERNET websocket = nullptr;  // for WebSocket
};
std::list<AsyncRequest> networkRequests;
void Tick() { 
    for (auto it = networkRequests.begin(); it != networkRequests.end(); ) {
        if (it->completed) {
            if (it->type == AsyncRequest::HTTP && it->httpCallback) {
                it->httpCallback(it->error == 0, it->response, it->error);
            } else if (it->type == AsyncRequest::WEBSOCKET_CONNECT && it->wsConnectCallback) {
                it->wsConnectCallback(it->error == 0, it->websocket, it->error);
            } else if (it->type == AsyncRequest::WEBSOCKET_SEND && it->wsSendCallback) {
                it->wsSendCallback(it->error == 0, it->error);
            }
            it = networkRequests.erase(it);
        } else {
            ++it;
        }
    }
}
```

### Decision 4: WebSocket 部分同步改造

**选择**：新增 `MakeWebSocketConnectionAsync` 和 `SendToWebsocketAsync`

**理由**：
- WebSocket 接收已有独立线程（StartWebSocketReceive）
- 只需改造连接和发送为异步 callback 模式

## Implementation Plan

### Phase 1: Network 层
1. 新增 `MakeHttpsRequestAsync`（WinHTTP Status Callback）
2. 新增 `MakeWebSocketConnectionAsync`
3. 新增 `SendToWebsocketAsync`

### Phase 2: Provider 层
1. 改造 MiniMaxAIChatProvider、DeepSeekAIChatProvider
2. 改造 MiniMaxTTSProvider、XiaomiTTSProvider
3. 改造 MimoTTSClient

### Phase 3: BliveManager
1. 新增 AsyncRequest 结构
2. 改造 Tick()、Start()、End()、StartAppHeartBeat()
3. 集成 WebSocket 异步接口

### Phase 4: Spec 同步
1. 更新 ai-chat-provider spec
2. 更新 mimo-tts-integration spec
3. 更新 tts-provider spec

### Phase 5: 验证
1. MSBuild 编译验证（Release x64）
2. 单元测试验证（Debug 配置，RUN_UNIT_TESTS）
3. 运行时功能验证（确认异步请求正常工作）

### Phase 6: 冗余代码清理
1. 删除协程版本 `MakeHttpsRequest`、`MakeWebSocketConnection`、`SendToWebsocket`
2. 删除 `NetworkCoroutine`、`HttpsAwaiter` 结构体
3. 删除 `BliveManager` 中的 `networkCoroutines` 成员和协程 resume 循环

**注意**：Phase 6 需在 Phase 5 验证通过后执行。

## Risks / Trade-offs

| 风险 | 描述 | 缓解措施 |
|------|------|---------|
| WinHTTP 异步 API 复杂 | 需要多次 callback 才能完成一次请求 | 参考 Network.cpp 已有异步实现（使用 WinHttpSetStatusCallback 注册回调） |
| Callback 生命周期 | 对象可能在 async 完成前销毁 | 使用 shared_ptr 管理状态 |
| 错误处理分散 | 异步错误需要传递到外层 | 统一通过 `onComplete(false, "", error)` 传递 |
| BliveManager 改造复杂度高 | 需要重构整个协程管理机制 | 分阶段实施，每阶段可运行验证 |

## Implementation Notes

### AsyncRequest 结构体

实现与 Decision 3 设计一致（简化版）：
- `enum Type { HTTP }`（WEBSOCKET_CONNECT/SEND 为死代码，已移除）
- `httpCallback` 成员
- `completed`, `response`, `error`, `websocket` 字段

额外保留：
- `Complete()` 方法：用于在异步操作完成时设置状态并标记 completed

注意：`HttpsAsyncContext` 新增 `cleanupDone` 原子标志防止双重 delete。

### HTTP 请求的 callback 模式

HTTP 请求（Provider 的 CallAPI、BliveManager 的 Start/End/StartAppHeartBeat）：
- Provider 内部使用 `condition_variable` 等待异步完成
- BliveManager 的 HTTP 请求通过 `AsyncRequest` 管理，callback 在 Provider 内部调用 `request->Complete()`
- `Tick()` 遍历 `networkRequests`，调用 type-specific callback，然后 erase completed 请求

### WebSocket 特殊处理

WebSocket 连接和发送：
- `MakeWebSocketConnectionAsync` 是完全异步的（内部创建线程）
- 连接成功后 `onConnect` callback 被立即调用
- 当前实现中 WebSocket callback 不经过 `networkRequests` 管理
- 这是因为 WebSocket 操作的异步特性与 HTTP 不同（连接建立后持续运行）

### 与原始设计的差异

| 方面 | 原始设计 | 实现 |
|------|---------|------|
| Tick() 驱动所有 callback | 是 | 仅 HTTP 请求通过 Tick() 处理 |
| Provider 等待方式 | Tick() 驱动 | 内部 cv.wait |
| WebSocket 管理 | networkRequests | 直接 callback |
| Session 复用 | 未规划 | HTTP 请求复用共享 Session（提高性能） |
| AsyncRequest Type 枚举 | HTTP + WEBSOCKET_CONNECT + WEBSOCKET_SEND | 仅 HTTP（后两者为死代码，已移除） |

### 实现过程中发现并修复的 Bug

| Bug | 严重性 | 描述 | 修复方案 |
|-----|--------|------|---------|
| ASYNC_RESULT 错误码提取错误 | Critical | `*(DWORD*)statusInfo` 应为 `((WINHTTP_ASYNC_RESULT*)statusInfo)->dwError`，且缺少长度检查 | 添加 `statusInfoLength >= sizeof(WINHTTP_ASYNC_RESULT)` 检查，使用 `->dwError` |
| HttpsAsyncContext 双重 delete | Critical | `HttpsStatusCallback` 错误回调线程和 `MakeHttpsRequestAsync` 主线程可能同时 delete ctx | 添加 `cleanupDone` 原子标志，确保只有一个线程执行 delete |
| WebSocket 句柄双重关闭 | Critical | `StartWebSocketReceive` 收到关闭帧后关闭句柄，但 `MakeWebSocketConnectionAsync` 仍传递已关闭句柄给调用者 | 移除 `StartWebSocketReceive` 中的 `WinHttpCloseHandle`，句柄所有权归调用者 |
| callback 异常安全 | High | 所有 async callback 调用缺少异常处理，lambda 异常会导致 `std::terminate` | 在所有 callback 调用外添加 `try-catch (...)` |
| WebSocket 消息体截断 | High | `WinHttpWebSocketReceive` 不保证一次返回完整 body，`bytesTransferred < bodyLength` 时仍处理不完整数据 | 添加 `bytesTransferred == bodyLength` 完整性检查 |
| Use-After-Free (UAF) 风险 | Critical | `StartWebSocketReceive` 启动 detached 线程调用 `BliveManager::Inst()->IsConnected()`，当 `Destroy()` 被调用时 `__Instance` 被置为 `nullptr`，但 detached 线程可能仍在访问已销毁对象 | 新增 `GetDestroyingFlag()` 到单例宏，销毁时设置标志，`IsConnected()` 检查该标志防止 UAF |
| EventSystem::Invoke 异常安全 | Medium | `Invoke` 遍历调用所有 handler 时，某 handler 异常会导致后续 handler 不被调用 | 在 handler 调用外添加 `try-catch (...)` |
| TextToSpeech 状态机竞态 | Medium | `ProcessPlayingState` 访问 `req` 对象时缺少锁保护，与 HTTP 回调线程存在竞态 | 添加 `asyncMutex_` 锁保护 |
| wsMsgLock 竞态条件 | Medium | `Disconnect()` 和 `~BliveManager()` 访问 `webSocket` 时缺少锁保护 | 添加 `wsMsgLock` 锁保护 |
| WebSocket header 完整性未验证 | Medium | `StartWebSocketReceive` 未验证 `bytesTransferred == 16` | 添加 `bytesTransferred != 16` 检查 |

### 多轮检查发现的额外问题（2026-04-07）

| Bug | 严重性 | 描述 | 修复方案 |
|-----|--------|------|---------|
| delayedTasks 并发安全问题 | Medium | `delayedTasks` 在多处被访问但无锁保护，与 `Tick()` 遍历存在竞态 | 添加 `delayedTasksLock` 锁保护所有访问 |
| networkRequests 迭代器失效 | Medium | `Tick()` 中 callback 执行期间，其他线程可能修改 `networkRequests` | 添加 `networkRequestsLock` 锁保护 |
| destroyed_ 死代码 | Low | `BliveManager.h` 中 `destroyed_` 成员变量未被使用 | 删除 `destroyed_` 成员变量 |

### PUA 多轮检查发现并修复的问题（2026-04-07）

| Bug | 严重性 | 描述 | 修复方案 |
|-----|--------|------|---------|
| WinHttpSetOption Context 指针错误 | Critical | 传递 `&ctx`（指针变量地址）而非 `ctx`（堆对象地址），导致回调接收到错误的上下文指针 | 修改为 `WinHttpSetOption(..., ctx, sizeof(ctx))` |
| REQUEST_ERROR 回调双重 callback | Critical | `WINHTTP_CALLBACK_STATUS_REQUEST_ERROR` 处理启动新线程调用 callback，但主线程 cleanup 块也会调用 callback | 移除 REQUEST_ERROR 分支中的线程启动，仅设置 completed=true，由主线程统一处理 |
| AsyncRequest::Complete 重复调用 | Critical | `Complete()` 无防重复检查，可能被多次调用导致 callback 被执行多次 | 添加 `if (completed) return;` 检查 |
| Tick() callback 死锁风险 | Critical | `Tick()` 在持有 `networkRequestsLock` 时调用 callback，如果 callback 内部调用 `Start()`/`End()` 等方法会尝试重新获取锁导致死锁 | 将 callback 收集到列表，锁外统一执行 |
| lambda 捕获悬空引用 | Critical | `auto& req = *it` 导致 lambda 按引用捕获，在 `erase(it)` 后 req 成为悬空引用 | 改为 `auto req = *it` 值拷贝 shared_ptr，延长生命周期 |

### Session 复用实现

`HttpsAsyncUtils::GetSharedSession()` 维护一个静态共享 Session：
- 首次调用时创建 Session，设置 5 秒超时
- 后续调用复用同一个 Session
- Session 在进程结束时自动释放（静态对象）
- 每次请求仍创建新的 Connect 和 Request，但复用 Session 可利用 WinHTTP 内部连接池
