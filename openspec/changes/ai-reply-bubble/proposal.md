# AI Reply Bubble Display - Proposal

## Why

目前舰长打卡 AI 功能只有 TTS 语音播报，没有视觉反馈。用户无法通过视觉快速了解 AI 回复内容。新增气泡显示功能可以在点怪窗口上方直观展示 AI 回复，提升用户体验。

## What Changes

在点怪窗口 (`OrderedMonsterWindow`) 上方添加气泡显示区域，当舰长发送打卡消息并触发 AI 回复时，在气泡中显示用户名和回复内容。气泡支持渐显/渐隐动画，新气泡从底部加入（离窗口最近），旧气泡向上移动被推远。

**气泡堆叠规则：**
- 新气泡始终添加在底部（离点怪窗口最近的位置）
- 旧气泡向上移动（新气泡高度 + 8px 间距）
- 超出 5 个时，最旧的气泡（最上方）被移除
- 窗口关闭时所有定时器停止并清理气泡

**量化参数：**
- 渐显动画：500ms (ease-out)
- 停留时间：5秒
- 渐隐动画：800ms (ease-out)
- 最大气泡数：5个
- 气泡间距：8px

**线程安全：**
- C++ 层使用 `std::mutex` 保护回调指针的读写
- 回调调用在锁外执行，避免长时间持有锁

## Capabilities

### New Capabilities

- `ai-reply-bubble`: 在点怪窗口上方显示 AI 回复气泡，支持动画效果

### Modified Capabilities

- `captain-checkin-ai`: 现有 AI 回复功能扩展，增加 UI 回调机制

## Design

### 气泡样式：紫蓝渐变背景

使用 `#667eea → #764ba2` 渐变背景，白色文字，用户名显示在顶部（小字、淡色 #CCFFFFFF）。

### 线程安全机制

C++ 层使用 `std::mutex` 保护回调指针的读写，回调调用在锁外执行，避免长时间持有锁。

## Data Flow

```
CaptainCheckInModule (C++)
    │
    │ PushDanmuEvent()
    │   └─→ IsCheckinMessage() → true
    │       └─→ GenerateCheckinAnswerSync()
    │           └─→ result.success
    │               └─→ g_aiReplyCallback(username, content, userData)
    │
    ▼
DataBridgeExports.cpp
    │
    │ DataBridge_SetAIReplyCallback()
    │   └─→ 设置 g_aiReplyCallback
    │
    ▼
NativeImports.cs (P/Invoke)
    │
    │ AIReplyCallback delegate
    │
    ▼
DanmuManager.cs
    │
    │ OnAIReplyCallback()
    │   └─→ GlobalEventListener.Invoke("AIReplyBubble", bubbleInfo)
    │
    ▼
OrderedMonsterWindow.xaml.cs
    │
    │ AddListener("AIReplyBubble", ...)
    │   └─→ BubbleCanvas.AddBubble()
```

## Risks

| 风险 | 缓解措施 |
|------|---------|
| 气泡过多可能导致窗口被顶出屏幕边缘 | 限制最大气泡数为 5 个 |
| 快速连续收到多个 AI 回复时，动画可能显得拥挤 | 气泡左对齐，紧贴窗口顶部 |
| `g_aiReplyCallback` 静态变量在多线程环境下需要考虑线程安全 | C++ 层使用 `std::mutex` 保护读写，回调调用在锁外执行 |
| C++ 回调传递的字符串指针需确保在回调完成前有效 | C++ 层使用局部 `std::wstring` 持有数据 |
| C++ 回调调用可能抛出未捕获异常 | C++ 回调调用包裹在 try-catch 中 |
| 窗口关闭时气泡定时器未停止导致内存泄漏或崩溃 | 窗口关闭时（Window.Closing 事件）停止所有 DispatcherTimer 并清理气泡 |
| 气泡内容过长时需要截断或换行处理 | 使用 TextBlock.TextWrapping=Wrap 并设置 MaxTextWidth |
| DispatcherTimer 动画完成后若未正确清理可能导致内存泄漏 | Dispatcher.InvokeAsync 确保 UI 操作在 UI 线程执行，定时器停止后设置 timer.Stop() |
| GlobalEventListener.Invoke 从 C++ 回调线程调用，存在跨线程 UI 调度安全性问题 | Dispatcher.InvokeAsync 确保 UI 操作在 UI 线程执行 |
| **关联风险**：`g_danmuProcessedCallback` 存在相同的线程安全问题 | 建议后续评估并统一处理 |

## Impact

- **C++ 层**: `CaptainCheckInModule` 需要在生成回复后调用 UI 回调
- **C# 层**: 新增 `AIBubbleInfo`、`AIBubbleControl` 组件
- **UI 层**: `OrderedMonsterWindow` 添加气泡覆盖层
- **无破坏性变更**: 现有功能保持不变