## Context

当前 BliveManager 使用简单的 `atomic<bool> connected` 来表示连接状态，只有 true/false 两种状态。断线重连逻辑分散在多处，每次断线都直接调用 `Start()` 尝试重连，没有重试限制。UI 只有"已连接"和"未连接"两种显示，用户无法了解连接过程和断连原因。

## Goals / Non-Goals

**Goals:**
- 实现完整的连接状态机，支持 5 种状态和断连原因追踪
- 实现带最大重试次数（5次）的重连策略
- 提供主动断开和手动重连的 API
- UI 显示连接状态、原因和重连进度
- 按钮根据状态切换文案

**Non-Goals:**
- 不修改现有的弹幕消息处理逻辑
- 不修改心跳的具体实现（保持 20 秒间隔）

## Decisions

### 1. 状态机设计

采用显式状态枚举而非简单的布尔值，便于追踪和显示。

```cpp
enum class ConnectionState {
    Disconnected,    // 未连接（初始状态）
    Connecting,      // 连接中（正在请求 /start API）
    Connected,       // 已连接（WebSocket 已建立）
    Reconnecting,   // 正在重连（断线后自动重连中）
    ReconnectFailed // 重连失败（达到最大重试次数）
};
```

**替代方案考虑：**
- 使用简单的布尔值 + 额外标志位 → 拒绝：状态转换逻辑复杂，难以追踪
- 使用状态模式（State Pattern）→ 拒绝：对于当前项目规模过于复杂

### 2. 断连原因追踪

```cpp
enum class DisconnectReason {
    None,            // 无/主动断开
    NetworkError,    // 网络错误（HTTP/WS 请求失败）
    HeartbeatTimeout,// 心跳超时
    ServerClose,    // 服务器主动关闭
    AuthFailed      // 鉴权失败
};
```

### 3. 重连策略

- **触发条件**：被动断连（网络错误、心跳超时、服务器关闭）
- **最大重试次数**：5 次
- **重连间隔**：立即重连（不等待）
- **重连失败处理**：停留在 ReconnectFailed 状态，等待用户手动重连

**替代方案考虑：**
- 指数退避 → 拒绝：需求明确为"立即重连"
- 无限重连 → 拒绝：可能造成服务器压力，用户也需要知道连接失败

### 4. 事件系统扩展

扩展现有事件，传递状态和原因：

```cpp
// 新事件：连接状态变化（包含原因）
Event<ConnectionState, DisconnectReason> OnConnectionStateChanged;

// 旧事件保留用于兼容
Event<> OnBliveConnected;    // 等价于 state == Connected
Event<> OnBliveDisconnected; // 等价于 state != Connected && reason != None
```

### 5. UI 按钮逻辑

| 当前状态 | 按钮文案 | 点击行为 |
|----------|----------|----------|
| Disconnected | 连接 | 调用 Start()，状态→Connecting |
| Connecting | 取消 | 调用 Disconnect()，状态→Disconnected |
| Connected | 断开 | 调用 Disconnect()，状态→Disconnected |
| Reconnecting | 取消 | 调用 Disconnect()，状态→Disconnected |
| ReconnectFailed | 连接 | 调用 Reconnect()，重置计数，状态→Connecting |

## Risks / Trade-offs

- **Risk**: 多处调用 Start() 需要改为根据当前状态决定行为
  - **Mitigation**: 在 Start() 开头添加状态检查，非 Disconnected 状态直接返回

- **Risk**: 现有代码多处设置 connected = true/false，需要统一修改
  - **Mitigation**: 新增 SetConnectionState() 方法统一管理状态转换

- **Risk**: 线程安全
  - **Mitigation**: ConnectionState 和 DisconnectReason 使用 atomic 或在锁内修改
