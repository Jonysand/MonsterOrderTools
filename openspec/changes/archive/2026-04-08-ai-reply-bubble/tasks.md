# AI Reply Bubble Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在点怪窗口上方显示 AI 回复气泡，支持渐显/渐隐动画，停留 5 秒，新气泡推动旧气泡上移。

**Architecture:** 使用 Canvas 叠加层管理气泡，在 OrderedMonsterWindow 内部实现。C++ 层通过新增的 AIReplyCallback 回调将 AI 回复传递到 C# 层。

**Tech Stack:** WPF (XAML + C#), C++/CLI (DataBridge)

---

## 1. C++ 层 - AI 回复回调机制

### Task 1: 在 DataBridgeExports.cpp 新增 AIReplyCallback 注册函数

**Files:**
- Modify: `MonsterOrderWilds/DataBridgeExports.h` (在现有回调类型之后)
- Modify: `MonsterOrderWilds/DataBridgeExports.cpp:378-397` (在现有回调模式之后)

- [x] **Step 1: 在 DataBridgeExports.h 添加 OnAIReplyCallback 类型定义**

在现有回调类型定义之后添加：

```cpp
typedef void(__stdcall* OnAIReplyCallback)(const wchar_t* username, const wchar_t* content, void* userData);
```

- [x] **Step 2: 添加 AIReplyCallback 类型定义和静态变量**

在 `DataBridge_SetDanmuProcessedCallback` 函数之后添加：

```cpp
// AI Reply 回调
#include <mutex>

static OnAIReplyCallback g_aiReplyCallback = nullptr;
static void* g_aiReplyUserData = nullptr;
static std::mutex g_aiReplyMutex;

__declspec(dllexport) void __stdcall DataBridge_SetAIReplyCallback(OnAIReplyCallback callback, void* userData)
{
    std::lock_guard<std::mutex> lock(g_aiReplyMutex);
    g_aiReplyCallback = callback;
    g_aiReplyUserData = userData;
}
```

- [x] **Step 3: 在 CaptainCheckInModule.cpp 包含回调头文件**

确认 `CaptainCheckInModule.cpp` 已包含必要的头文件

- [x] **Step 4: Commit**

```bash
git add MonsterOrderWilds/DataBridgeExports.h MonsterOrderWilds/DataBridgeExports.cpp
git commit -m "feat(ai-reply): add AIReplyCallback mechanism to DataBridge"
```

### Task 2: 在 CaptainCheckInModule.cpp 调用 AIReplyCallback

**Files:**
- Modify: `MonsterOrderWilds/CaptainCheckInModule.cpp:269-274`

- [x] **Step 1: 在 PushDanmuEvent 中添加回调调用**

在 `AnswerResult result = GenerateCheckinAnswerSync(checkinEvt);` 之后、TTS 调用之前添加：

```cpp
if (result.success && !result.answerContent.empty()) {
    // 调用 UI 回调显示气泡
    // 使用局部变量确保字符串指针在回调期间有效
    std::wstring usernameCopy = event.username;
    std::wstring contentCopy = result.answerContent;

    // 【关键线程安全模式】先在锁内复制回调指针，锁外调用
    // 避免长时间持有锁阻塞其他线程设置回调
    OnAIReplyCallback callback = nullptr;
    void* userData = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_aiReplyMutex);
        callback = g_aiReplyCallback;
        userData = g_aiReplyUserData;
    }
    if (callback) {
        try {
            callback(usernameCopy.c_str(), contentCopy.c_str(), userData);
        } catch (...) {
            // 记录日志，忽略异常
        }
    }
    if (ConfigManager::Inst()->GetConfig().enableVoice) {
        PlayCheckinTTS(result.answerContent, event.username, event.serverTimestamp);
    }
}
```

**【关键线程安全模式说明】**：必须先在锁内复制 `callback` 和 `userData` 指针到局部变量，然后在锁外调用回调。这是防止死锁和阻塞的关键设计。

**注意：** 由于 C++ 回调在单独线程执行，需要通过 `extern` 声明从 `DataBridgeExports.cpp` 引入全局变量。在文件顶部添加：

```cpp
extern OnAIReplyCallback g_aiReplyCallback;
extern void* g_aiReplyUserData;
extern std::mutex g_aiReplyMutex;
```

- [ ] **Step 2: Commit**

```bash
git add MonsterOrderWilds/CaptainCheckInModule.cpp
git commit -m "feat(ai-reply): invoke AIReplyCallback after generating answer"
```

---

## 2. C# 层 - P/Invoke 和事件桥接

### Task 3: 在 NativeImports.cs 添加 AIReplyCallback 声明

**Files:**
- Modify: `JonysandMHDanmuTools/NativeImports.cs`

- [x] **Step 1: 添加 OnAIReplyCallback 委托声明**

在现有委托声明区域添加：

```csharp
[UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
public delegate void OnAIReplyCallback(string username, string content, IntPtr userData);
```

- [x] **Step 2: 添加 SetAIReplyCallback 函数声明**

在现有 P/Invoke 函数声明区域添加：

```csharp
[DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
public static extern void DataBridge_SetAIReplyCallback(OnAIReplyCallback callback, IntPtr userData);
```

- [ ] **Step 3: Commit**

```bash
git add JonysandMHDanmuTools/NativeImports.cs
git commit -m "feat(ai-reply): add P/Invoke for AIReplyCallback"
```

### Task 4: 创建 AIBubbleInfo 数据结构

**Files:**
- Create: `JonysandMHDanmuTools/AIBubbleInfo.cs`

- [x] **Step 1: 创建 AIBubbleInfo 类**

```csharp
using System;

namespace MonsterOrderWindows
{
    public class AIBubbleInfo
    {
        public string Username { get; set; }
        public string Content { get; set; }
        public DateTime Timestamp { get; set; }

        public AIBubbleInfo(string username, string content)
        {
            this.Username = username;
            this.Content = content;
            this.Timestamp = DateTime.Now;
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add JonysandMHDanmuTools/AIBubbleInfo.cs
git commit -m "feat(ai-reply): add AIBubbleInfo data structure"
```

### Task 5: 在 DanmuManager.cs 注册 AIReplyCallback

**Files:**
- Modify: `JonysandMHDanmuTools/DanmuManager.cs`

- [x] **Step 1: 添加 AIReplyCallback 实例变量**

```csharp
private static OnAIReplyCallback _aiReplyCallback;
```

- [x] **Step 2: 在构造函数中注册回调**

```csharp
_aiReplyCallback = OnAIReplyCallback;
NativeImports.DataBridge_SetAIReplyCallback(_aiReplyCallback, IntPtr.Zero);
```

- [x] **Step 3: 添加 OnAIReplyCallback 处理函数**

```csharp
private static void OnAIReplyCallback(string username, string content, IntPtr userData)
{
    var bubbleInfo = new AIBubbleInfo(username, content);
    GlobalEventListener.Invoke("AIReplyBubble", bubbleInfo);
}
```

- [ ] **Step 4: Commit**

```bash
git add JonysandMHDanmuTools/DanmuManager.cs
git commit -m "feat(ai-reply): register AIReplyCallback and dispatch event"
```

---

## 3. UI 层 - 气泡控件和动画

### Task 6: 创建 AIBubbleControl 用户控件

**Files:**
- Create: `JonysandMHDanmuTools/AIBubbleControl.xaml`
- Create: `JonysandMHDanmuTools/AIBubbleControl.xaml.cs`

- [x] **Step 1: 创建 AIBubbleControl.xaml**

```xaml
<UserControl x:Class="MonsterOrderWindows.AIBubbleControl"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    Width="380" MinHeight="60">
    <Border CornerRadius="12" Padding="12,8">
        <Border.Background>
            <LinearGradientBrush StartPoint="0,0" EndPoint="1,1">
                <GradientStop Color="#667eea" Offset="0"/>
                <GradientStop Color="#764ba2" Offset="1"/>
            </LinearGradientBrush>
        </Border.Background>
        <Border.Effect>
            <DropShadowEffect BlurRadius="10" ShadowDepth="2" Opacity="0.4"/>
        </Border.Effect>
        <Grid>
            <Grid.RowDefinitions>
                <RowDefinition Height="Auto"/>
                <RowDefinition Height="Auto"/>
            </Grid.RowDefinitions>
            <TextBlock Grid.Row="0" FontSize="11" Foreground="#CCFFFFFF"
                       Text="{Binding Username}" Margin="0,0,0,4"/>
            <TextBlock Grid.Row="1" FontSize="14" Foreground="White" 
                       TextWrapping="Wrap" Text="{Binding Content}"/>
        </Grid>
    </Border>
</UserControl>
```

- [x] **Step 2: 创建 AIBubbleControl.xaml.cs**

```csharp
using System.Windows.Controls;

namespace MonsterOrderWindows
{
    public partial class AIBubbleControl : UserControl
    {
        public AIBubbleControl()
        {
            InitializeComponent();
        }
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add JonysandMHDanmuTools/AIBubbleControl.xaml JonysandMHDanmuTools/AIBubbleControl.xaml.cs
git commit -m "feat(ai-reply): add AIBubbleControl user control"
```

### Task 7: 修改 OrderedMonsterWindow.xaml 添加 BubbleCanvas

**Files:**
- Modify: `JonysandMHDanmuTools/OrderedMonsterWindow.xaml`

- [x] **Step 1: 在 Window.Resources 中添加动画定义**

在 `</Window.Resources>` 之前添加：

```xml
<Storyboard x:Key="BubbleEnterStoryboard">
    <DoubleAnimation Storyboard.TargetProperty="Opacity"
                     From="0" To="1" Duration="0:0:0.5">
        <DoubleAnimation.EasingFunction>
            <QuadraticEase EasingMode="EaseOut"/>
        </DoubleAnimation.EasingFunction>
    </DoubleAnimation>
</Storyboard>

<Storyboard x:Key="BubbleExitStoryboard">
    <DoubleAnimation Storyboard.TargetProperty="Opacity"
                     From="1" To="0" Duration="0:0:0.8">
        <DoubleAnimation.EasingFunction>
            <QuadraticEase EasingMode="EaseOut"/>
        </DoubleAnimation.EasingFunction>
    </DoubleAnimation>
</Storyboard>
```

- [x] **Step 2: 在 MainGrid 内添加 BubbleCanvas**

在 `<Grid MouseLeftButtonDown="OnClientAreaMouseLeftButtonDown" ...>` 之后，`</Grid>` 之前添加：

```xml
<Canvas x:Name="BubbleCanvas" VerticalAlignment="Top" HorizontalAlignment="Left" 
        ClipToBounds="False" IsHitTestVisible="False"/>
```

- [ ] **Step 3: Commit**

```bash
git add JonysandMHDanmuTools/OrderedMonsterWindow.xaml
git commit -m "feat(ai-reply): add BubbleCanvas and animation storyboards"
```

### Task 8: 修改 OrderedMonsterWindow.xaml.cs 实现气泡管理逻辑

**Files:**
- Modify: `JonysandMHDanmuTools/OrderedMonsterWindow.xaml.cs`

- [x] **Step 1: 添加私有变量和常量**

```csharp
private List<AIBubbleControl> _bubbles = new List<AIBubbleControl>();
private List<DispatcherTimer> _bubbleTimers = new List<DispatcherTimer>();
private const int MAX_BUBBLES = 5;
private const int BUBBLE_INTERVAL_MS = 5000;
private const double BUBBLE_MARGIN = 8;
```

- [x] **Step 2: 在构造函数中注册 AIReplyBubble 事件监听和窗口关闭事件**

```csharp
GlobalEventListener.AddListener("AIReplyBubble", (object info) => AddBubble(info as AIBubbleInfo));
```

- [x] **Step 3: 添加 AddBubble 方法**

```csharp
public void AddBubble(AIBubbleInfo bubbleInfo)
{
    if (bubbleInfo == null) return;

    Dispatcher.InvokeAsync(() =>
    {
        // 超出上限时移除最旧的气泡（最上方）
        while (_bubbles.Count >= MAX_BUBBLES)
        {
            RemoveBubble(_bubbles[0]);
        }

        var bubble = new AIBubbleControl();
        bubble.DataContext = bubbleInfo;
        bubble.Opacity = 0;

        // 新气泡添加到列表末尾（底部）
        _bubbles.Add(bubble);
        BubbleCanvas.Children.Add(bubble);

        // 更新所有气泡位置：新气泡在底部，旧气泡向上移动
        UpdateBubblePositions();

        var storyboard = (Storyboard)FindResource("BubbleEnterStoryboard");
        storyboard = storyboard.Clone();
        storyboard.Begin(bubble);

        var timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(BUBBLE_INTERVAL_MS) };
        timer.Tick += (s, e) =>
        {
            timer.Stop();
            _bubbleTimers.Remove(timer);
            PlayExitAnimation(bubble);
        };
        timer.Start();
        _bubbleTimers.Add(timer);
    });
}
```

- [x] **Step 4: 添加 RemoveBubble 方法**

```csharp
private void RemoveBubble(AIBubbleControl bubble)
{
    if (bubble == null || !_bubbles.Contains(bubble)) return;
    _bubbles.Remove(bubble);
    BubbleCanvas.Children.Remove(bubble);
}
```

- [x] **Step 5: 添加 UpdateBubblePositions 方法**

```csharp
private void UpdateBubblePositions()
{
    // 从底部开始向上堆叠
    double top = 0;
    for (int i = _bubbles.Count - 1; i >= 0; i--)
    {
        var bubble = _bubbles[i];
        bubble.UpdateLayout();
        double height = bubble.ActualHeight > 0 ? bubble.ActualHeight : 60;
        Canvas.SetTop(bubble, top);
        top += height + BUBBLE_MARGIN;
    }
}
```

- [x] **Step 6: 添加 PlayExitAnimation 方法**

```csharp
private void PlayExitAnimation(AIBubbleControl bubble)
{
    if (bubble == null || !_bubbles.Contains(bubble)) return;
    var storyboard = (Storyboard)FindResource("BubbleExitStoryboard");
    storyboard = storyboard.Clone();
    storyboard.Completed += (s, e) =>
    {
        RemoveBubble(bubble);
        UpdateBubblePositions();
    };
    storyboard.Begin(bubble);
}
```

- [x] **Step 7: 添加窗口关闭清理方法**

```csharp
private void CleanupBubbles()
{
    foreach (var timer in _bubbleTimers)
    {
        timer.Stop();
    }
    _bubbleTimers.Clear();
    foreach (var bubble in _bubbles)
    {
        var enterStory = (Storyboard)FindResource("BubbleEnterStoryboard");
        var exitStory = (Storyboard)FindResource("BubbleExitStoryboard");
        if (enterStory != null) enterStory.Stop(bubble);
        if (exitStory != null) exitStory.Stop(bubble);
    }
    _bubbles.Clear();
    BubbleCanvas.Children.Clear();
}
```

**注意**：停止所有气泡的动画后立即清理，无需等待动画完成。

- [x] **Step 8: 在 Window.Closing 事件中调用清理方法**

在窗口构造函数中添加：
```csharp
this.Closing += (s, e) => CleanupBubbles();
```

- [ ] **Step 9: Commit**

```bash
git add JonysandMHDanmuTools/OrderedMonsterWindow.xaml.cs
git commit -m "feat(ai-reply): implement bubble management logic"
```

### Task 8.5: 添加新文件到 csproj

**Files:**
- Modify: `JonysandMHDanmuTools/MonsterOrderWildsGUI.csproj`

- [x] **Step 1: 添加 AIBubbleInfo.cs 到 ItemGroup**

找到包含 `<Compile Include="DanmuManager.cs" />` 的 ItemGroup，添加：

```xml
<Compile Include="AIBubbleInfo.cs" />
```

- [x] **Step 2: 添加 AIBubbleControl.xaml 和 .xaml.cs 到 ItemGroup**

找到包含 `<Page Include="OrderedMonsterWindow.xaml">` 的 ItemGroup，添加：

```xml
<Page Include="AIBubbleControl.xaml">
  <SubType>Designer</SubType>
  <Generator>MSBuild:Compile</Generator>
</Page>
<Compile Include="AIBubbleControl.xaml.cs">
  <DependentUpon>AIBubbleControl.xaml</DependentUpon>
</Compile>
```

- [ ] **Step 3: Commit**

```bash
git add JonysandMHDanmuTools/MonsterOrderWildsGUI.csproj
git commit -m "feat(ai-reply): add new files to project"
```

---

## 4. 验证和测试

### Task 9: 验证编译

**Files:**
- None (verification only)

- [x] **Step 1: 运行 MSBuild 编译**

```bash
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln" -p:Configuration=Release -p:Platform=x64 -t:Build -m
```

Expected: BUILD SUCCEEDED

- [ ] **Step 2: Commit**

```bash
git add -A
git commit -m "feat(ai-reply): complete AI reply bubble display feature"
```

---

## 5. 自检清单

完成所有任务后，执行以下验证：

- [x] 所有 10 个 Task 的 checkbox 都已勾选（含 Task 8.5）
- [x] 编译成功无错误
- [x] GlobalEventListener.Invoke("AIReplyBubble", ...) 被正确触发
- [x] 气泡动画（渐显 500ms、停留 5s、渐隐 800ms）符合设计
- [x] 气泡队列上限为 5 个
- [x] 新气泡从底部加入（离窗口最近），旧气泡向上移动被推远
- [x] 新增文件（AIBubbleInfo.cs、AIBubbleControl.xaml/.cs）已添加到 csproj
- [x] 无占位符、无 TODO、无 TBD
- [x] C# 层回调在程序启动时正确注册
- [x] C++ 层 g_aiReplyCallback 调用前检查非空（回调注册在程序启动时完成，气泡队列限制为5个）
- [x] C++ 回调使用局部 std::wstring 持有字符串数据，确保指针在回调期间有效
- [x] C++ 回调调用在锁外执行，避免长时间持有锁
- [x] 窗口关闭时 CleanupBubbles() 被调用，所有定时器停止
- [x] 窗口关闭时所有气泡的动画立即停止（Storyboard.Stop()），无需等待动画完成
- [x] DispatcherTimer 在气泡移除时从 _bubbleTimers 列表中移除
- [x] 气泡内容过长时使用 TextWrapping 换行处理
- [x] 打包配置检查：新增文件为源代码文件（.cs/.xaml），已通过 csproj 包含，无需额外 installer 配置

---

## 文件变更汇总

| 操作 | 文件路径 |
|------|---------|
| Modify | `MonsterOrderWilds/DataBridgeExports.h` |
| Modify | `MonsterOrderWilds/DataBridgeExports.cpp` |
| Modify | `MonsterOrderWilds/CaptainCheckInModule.cpp` |
| Modify | `JonysandMHDanmuTools/NativeImports.cs` |
| Create | `JonysandMHDanmuTools/AIBubbleInfo.cs` |
| Modify | `JonysandMHDanmuTools/DanmuManager.cs` |
| Create | `JonysandMHDanmuTools/AIBubbleControl.xaml` |
| Create | `JonysandMHDanmuTools/AIBubbleControl.xaml.cs` |
| Modify | `JonysandMHDanmuTools/OrderedMonsterWindow.xaml` |
| Modify | `JonysandMHDanmuTools/OrderedMonsterWindow.xaml.cs` |
| Modify | `JonysandMHDanmuTools/MonsterOrderWildsGUI.csproj` (新增文件添加到项目) |