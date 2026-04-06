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
