## Context

当前实现中，点怪窗口（OrderedMonsterWindow）的透明度由单一 `opacity` 配置控制。穿透模式（锁定窗口）下的透明度按固定比例（170/255 ≈ 67%）计算。用户无法独立控制两种模式的透明度。

## Goals / Non-Goals

**Goals:**
- 新增 `penetratingModeOpacity` 配置字段
- 保留现有 `opacity` 字段作为非穿透模式透明度
- UI 新增穿透模式透明度滑块
- 业务逻辑根据窗口锁定状态选择对应透明度

**Non-Goals:**
- 不修改现有的窗口锁定/解锁逻辑
- 不修改透明度的 Alpha 值计算方式（统一使用 255 作为基准）
- 不添加新的持久化机制

## Decisions

### 1. 新增配置字段

在 `ConfigData` 中新增 `penetratingModeOpacity`（int，默认值 50），与现有 `opacity`（默认值 80）并列。

### 2. 字段注册

在 `ConfigFieldRegistry.cpp` 中注册 `penetratingModeOpacity` 字段，类型为 `Int`。

### 3. 数据桥接

`DataBridgeWrapper.h` 新增 `PenetratingModeOpacity` 属性，C# 层 `ConfigDataSnapshot` 和 `MainConfig` 同步新增字段。

### 4. UI 设计

在 ConfigWindow.xaml 现有透明度滑块下方，新增"穿透模式透明度"滑块：
- Label: "穿透模式透明度"
- Slider: Name="PenetratingModeOpacitySlider", Minimum=0, Maximum=100, Value=50
- TextBlock: 显示当前值

### 5. 业务逻辑变更

`OrderedMonsterWindow.xaml.cs` 的 `RefreshWindow()` 方法修改：
```csharp
float opacity = mIsLocked 
    ? ToolsMain.GetConfigService().Config.PENETRATING_MODE_OPACITY / 100f
    : ToolsMain.GetConfigService().Config.OPACITY / 100f;
int alphaInt = (int)(255f * opacity + 0.5f);
```

## Risks / Trade-offs

- **风险**: 用户更新后，已有穿透模式用户会发现窗口变得更透明（从隐式的 67% 变为显式的 50%）
- **权衡**: 默认值 50 是一个合理的折中，既保持穿透模式的透明特性，又给予用户更大的控制自由度

## Implementation Notes

### VEH 移除

在 Debug 测试过程中发现 VEH (Vectored Exception Handler) 代码导致调试异常捕获混乱。为便于调试，移除了 `main.cpp` 中的 VEH 相关代码：
- 删除 `GlobalVectoredExceptionHandler` 函数
- 删除 `RegisterGlobalVEH()` 函数
- 删除 `wWinMain()` 中的 `RegisterGlobalVEH()` 调用

此修改不影响程序功能，仅用于提升调试体验。
