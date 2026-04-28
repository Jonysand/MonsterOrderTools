## Why

当前弹幕连接的状态管理过于简单，只有"已连接"和"未连接"两种状态，无法让用户了解真实的连接过程和断连原因。断线重连逻辑缺乏健壮性，没有重试限制和主动控制能力，影响用户体验。

## What Changes

- 新增连接状态枚举：Disconnected、Connecting、Connected、Reconnecting、ReconnectFailed
- 新增断连原因枚举：None、NetworkError、HeartbeatTimeout、ServerClose、AuthFailed
- 实现指数退避重连策略，最大重试次数 5 次
- 添加主动断开连接方法 Disconnect()
- 添加手动重连方法 Reconnect()
- 扩展事件系统，传递状态和断连原因
- 更新 UI 显示连接状态、原因和重连进度
- 添加断开/重连按钮，根据状态切换文案

## Capabilities

### New Capabilities
- `connection-state-machine`: 连接状态机管理，包括状态转换、原因追踪

## Impact

- `BliveManager.h/cpp`: 新增状态枚举、状态变量、方法声明和实现
- `MonsterOrderWilds.cpp`: 事件处理更新
- `MHDanmuToolsHost.h`: 事件签名更新
- `ToolsMain.cs`: UI 状态更新
- `ConfigWindow.xaml`: 新按钮、新状态显示
- `ConfigWindow.xaml.cs`: 按钮事件处理
