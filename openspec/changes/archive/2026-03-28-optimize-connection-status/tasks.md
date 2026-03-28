## 1. BliveManager 核心改动

- [x] 1.1 在 BliveManager.h 中添加 ConnectionState 枚举（Disconnected, Connecting, Connected, Reconnecting, ReconnectFailed）
- [x] 1.2 在 BliveManager.h 中添加 DisconnectReason 枚举（None, NetworkError, HeartbeatTimeout, ServerClose, AuthFailed）
- [x] 1.3 在 BliveManager.h 中添加 connectionState 和 disconnectReason 成员变量及原子操作方法
- [x] 1.4 在 BliveManager.h 中添加 maxReconnectAttempts 常量（5）和 reconnectAttemptCount 计数器
- [x] 1.5 在 BliveManager.h 中添加 Disconnect() 和 Reconnect() 方法声明
- [x] 1.6 在 BliveManager.h 中添加 OnConnectionStateChanged 事件声明（传递 state 和 reason）
- [x] 1.7 在 BliveManager.cpp 中实现 SetConnectionState() 方法，统一管理状态转换
- [x] 1.8 在 BliveManager.cpp 中修改 Start() 方法，支持重连模式（传入 reason 参数）
- [x] 1.9 在 BliveManager.cpp 中实现 Disconnect() 方法，主动断开并清除重试计数
- [x] 1.10 在 BliveManager.cpp 中实现 Reconnect() 方法，重置计数并触发连接
- [x] 1.11 修改 OnReceiveStartResponse()，成功时调用 SetConnectionState(Connected)
- [x] 1.12 修改 OnReceiveAppHeartbeatResponse()，失败时触发重连逻辑
- [x] 1.13 修改 HandleWSMessage()，根据不同 OP 码设置正确的 disconnectReason 并触发重连
- [x] 1.14 修改 HandleSmsReply()，处理 INTERACTION_END 消息时设置 ServerClose 原因

## 2. C++ 层事件桥接

- [x] 2.1 修改 MHDanmuToolsHost.h，添加 OnConnectionStateChanged 事件处理（传递 state 和 reason）
- [x] 2.2 修改 MonsterOrderWilds.cpp，监听 OnConnectionStateChanged 事件并转发到 ToolsMainHost

## 3. C# 层 UI 支持

- [x] 3.1 在 ToolsMain.cs 中添加 OnConnectionStateChanged 方法（接收 state 和 reason 参数）
- [x] 3.2 在 ToolsMain.cs 中添加 GetConnectionState() 和 GetDisconnectReason() 供 UI 查询
- [x] 3.3 在 ConfigWindow.xaml 中添加连接按钮（ConnectButton）
- [x] 3.4 在 ConfigWindow.xaml 中修改状态显示区域，显示原因和重连进度
- [x] 3.5 在 ConfigWindow.xaml.cs 中添加 ConnectButton_Click 事件处理
- [x] 3.6 在 ConfigWindow.xaml.cs 中添加 UpdateConnectionUI() 方法，根据状态更新按钮文案和状态显示

## 4. 单元测试

- [x] 4.1 创建 BliveManagerTests.cpp 测试文件和 BliveManagerTests.vcxproj
- [x] 4.2 添加 ConnectionState 枚举测试（初始状态为 Disconnected）
- [x] 4.3 添加 SetConnectionState() 状态转换测试
- [x] 4.4 添加 DisconnectReason 枚举测试
- [x] 4.5 添加重连计数测试（初始为 0）
- [x] 4.6 添加 Disconnect() 方法测试（清除重连计数、设置 reason 为 None）
- [x] 4.7 添加 Reconnect() 方法测试（从 ReconnectFailed 恢复到 Connecting）
- [x] 4.8 添加自动重连触发测试（断连时增加计数）
- [x] 4.9 添加重连上限测试（达到 5 次后转为 ReconnectFailed）

**测试运行结果**：所有 34 个单元测试全部通过

## 5. 测试与验证

- [x] 5.1 编译项目确保无错误
- [x] 5.2 测试正常连接流程
- [x] 5.3 测试主动断开功能
- [x] 5.4 测试断线自动重连（5次限制）
- [x] 5.5 5次重连失败后显示重连失败状态
- [x] 5.6 测试手动重连功能
