# AI Reply Bubble Display - Design

## Context

舰长打卡 AI 功能 (`CaptainCheckInModule`) 目前通过 TTS 语音播报 AI 回复，但缺少视觉反馈。用户需要在点怪窗口上方直观看到 AI 回复内容。

## Goals / Non-Goals

**Goals:**
- 在点怪窗口上方显示渐变气泡
- 气泡支持渐显/渐隐动画，停留 5 秒
- 新气泡推动旧气泡上移
- 最多同时显示 5 个气泡

**Non-Goals:**
- 不改变现有 AI 回复生成逻辑
- 不修改 TTS 功能
- 不支持用户交互（点击等）

## Decisions

### 1. 使用 Canvas 叠加层作为气泡容器

**Decision:** 在 `OrderedMonsterWindow` 内部添加 `Canvas` 覆盖层管理气泡

**Reasoning:**
- 性能最优：单一 Canvas 元素，WPF 硬件加速
- 便于管理气泡位置：使用 `Canvas.SetTop()` 控制垂直堆叠
- 避免 Popup 的 HWND 开销

### 2. 气泡样式：紫蓝渐变背景

**Decision:** 使用 `#667eea → #764ba2` 渐变背景，白色文字，用户名显示在顶部（小字、淡色 #CCFFFFFF），气泡最大宽度 380px，TextWrapping 自动换行

**Reasoning:**
- 与暗色窗口形成对比，视觉醒目
- 紫蓝渐变是现代 UI 常见风格
- 半透明效果与窗口风格协调
- 用户名淡色小字区分层次，回复内容白色醒目

### 3. 动画时长配置

**Decision:**
- 渐显: 500ms (ease-out)
- 停留: 5000ms
- 渐隐: 800ms (ease-out)
- 气泡间距: 8px

**Reasoning:**
- 500ms 渐显足够快，不会延迟用户看到内容
- 800ms 渐隐相对慢一些，营造自然消失效果
- 5 秒停留确保用户有足够时间阅读
- 8px 间距提供足够视觉分离又不浪费空间

### 4. C++ → C# 回调机制

**Decision:** 新增 `DataBridge_SetAIReplyCallback` P/Invoke 函数

**Reasoning:**
- 与现有 `DataBridge_SetDanmuProcessedCallback` 模式一致
- 回调函数指针传递给 C++ 层，由 C++ 在合适的时机调用
- 保持架构一致性

### 5. 气泡数据结构

**Decision:** 新增 `AIBubbleInfo` 类存储气泡数据

**Reasoning:**
- 解耦数据与 UI：MVVM 模式
- 便于扩展（如添加头像、时间戳等）

## Risks / Trade-offs

**Risks:**
- 气泡过多可能导致窗口被顶出屏幕边缘
- 快速连续收到多个 AI 回复时，动画可能显得拥挤
- `g_aiReplyCallback` 静态变量在多线程环境下需要考虑线程安全
- C++ 回调传递的字符串指针需确保在回调完成前有效
- C++ 回调调用可能抛出未捕获异常
- 窗口关闭时气泡定时器未停止导致内存泄漏或崩溃
- 气泡内容过长时需要截断或换行处理
- DispatcherTimer 动画完成后若未正确清理可能导致内存泄漏
- GlobalEventListener.Invoke 从 C++ 回调线程调用，存在跨线程 UI 调度安全性问题
- **关联风险**：`g_danmuProcessedCallback` 存在与 `g_aiReplyCallback` 相同的线程安全问题，但本次 change 未涉及

**Mitigation:**
- 限制最大气泡数为 5 个
- 气泡左对齐，紧贴窗口顶部
- C++ 层使用 `std::mutex` 保护 `g_aiReplyCallback` 和 `g_aiReplyUserData` 的读写
- **回调调用在锁外执行**：先在锁内复制回调指针和用户数据，锁外调用回调，避免长时间持有锁阻塞其他线程
- C++ 层使用局部 `std::wstring` 持有数据，确保指针在回调期间有效
- C++ 回调调用包裹在 try-catch 中，捕获所有异常
- C# 层在程序启动时优先注册回调，确保回调在首次使用前已注册
- 窗口关闭时（Window.Closing 事件）停止所有 DispatcherTimer 并清理气泡
- 气泡内容使用 TextBlock.TextWrapping=Wrap 并设置 MaxTextWidth 防止无限延展
- Dispatcher.InvokeAsync 确保 UI 操作在 UI 线程执行
- 定时器停止后设置 timer.Stop() 并释放引用
- **后续建议**：评估 `g_danmuProcessedCallback` 的线程安全性，使用相同模式进行保护

## File Structure

```
JonysandMHDanmuTools/
├── AIBubbleInfo.cs                    [NEW] 气泡数据结构
├── AIBubbleControl.xaml               [NEW] 气泡用户控件
├── AIBubbleControl.xaml.cs            [NEW]
├── NativeImports.cs                    [MOD] 新增 AIReplyCallback 声明
├── DanmuManager.cs                     [MOD] 注册 AIReplyCallback
├── OrderedMonsterWindow.xaml           [MOD] 添加 BubbleCanvas
├── OrderedMonsterWindow.xaml.cs        [MOD] 气泡管理逻辑
└── MonsterOrderWildsGUI.csproj        [MOD] 添加新文件到项目

MonsterOrderWilds/
├── DataBridgeExports.h                [MOD] 新增 OnAIReplyCallback 类型定义
├── DataBridgeExports.cpp              [MOD] 新增 SetAIReplyCallback
└── CaptainCheckInModule.cpp           [MOD] 调用 AIReplyCallback
```

## Animation Specification

### Enter Animation
```xaml
<Storyboard x:Key="BubbleEnterStoryboard">
    <DoubleAnimation Storyboard.TargetProperty="Opacity"
                     From="0" To="1" Duration="0:0:0.5">
        <DoubleAnimation.EasingFunction>
            <QuadraticEase EasingMode="EaseOut"/>
        </DoubleAnimation.EasingFunction>
    </DoubleAnimation>
</Storyboard>
```

### Exit Animation
```xaml
<Storyboard x:Key="BubbleExitStoryboard">
    <DoubleAnimation Storyboard.TargetProperty="Opacity"
                     From="1" To="0" Duration="0:0:0.8">
        <DoubleAnimation.EasingFunction>
            <QuadraticEase EasingMode="EaseOut"/>
        </DoubleAnimation.EasingFunction>
    </DoubleAnimation>
</Storyboard>
```

## Bubble Layout

```
┌─────────────────────────────────────────┐
│ BubbleCanvas (Canvas)                   │
│                                         │
│  ┌─────────────────────┐               │
│  │ [Bubble 5 - 最旧]   │ ← 超出时被移除 │
│  └─────────────────────┘               │
│           ↑                            │
│    8px 间距向上推动                     │
│           ↑                            │
│  ┌─────────────────────┐               │
│  │ [Bubble 4]          │               │
│  └─────────────────────┘               │
│           ↑                            │
│  ┌─────────────────────┐               │
│  │ [Bubble 3]          │               │
│  └─────────────────────┘               │
│           ...                          │
│  ┌─────────────────────┐               │
│  │ [Bubble 2]          │               │
│  └─────────────────────┘               │
│           ↑                            │
│  ┌─────────────────────┐               │
│  │ [Bubble 1 - 最新]   │ ← 新气泡加入位置(底部) │
│  └─────────────────────┘               │
│                                         │
├─────────────────────────────────────────┤
│ OrderedMonsterWindow                    │
│ (跑马灯 + ListView)                     │
└─────────────────────────────────────────┘
```

**位置说明**：
- 新气泡始终添加在底部（所有气泡的最下方）
- 新气泡加入后，所有旧气泡向上移动（Y 坐标减小：新气泡高度 + 8px 间距）
- 气泡从底部向上的顺序为：最新 → 最旧
- 超出 5 个时，最旧的气泡（最上方）被移除

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
    │       └─→ 创建 AIBubbleControl
    │       └─→ 播放 Enter 动画
    │       └─→ 启动 5s 定时器
    │           └─→ 播放 Exit 动画
    │           └─→ 从 Canvas 移除
```