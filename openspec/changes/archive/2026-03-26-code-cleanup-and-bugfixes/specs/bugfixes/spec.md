## ADDED Requirements

### Requirement: 锁定窗口按钮与热键状态同步
系统 SHALL 通过事件机制确保锁定窗口按钮和热键的状态同步。

#### Scenario: 点击按钮锁定
- **WHEN** 用户点击"锁定窗口"按钮
- **THEN** 按钮文本变为"解锁窗口"，窗口进入锁定状态
- **AND** `LockStateChanged` 事件被触发

#### Scenario: 使用热键锁定
- **WHEN** 用户使用热键触发锁定
- **THEN** 按钮文本变为"解锁窗口"，窗口进入锁定状态
- **AND** `LockStateChanged` 事件被触发

#### Scenario: 初始化配置窗口
- **WHEN** 首次打开配置窗口
- **THEN** 锁定按钮文本根据当前 `mIsLocked` 状态初始化

### Requirement: 窗口解锁位运算正确性
系统 SHALL 使用 AND-NOT 操作清除 `WS_EX_TRANSPARENT` 标志位，而非 XOR 翻转。

#### Scenario: 解锁窗口
- **WHEN** 用户触发解锁（`mIsLocked` 从 true 变为 false）
- **THEN** `SetWindowLong(hwnd, -20, extendedStyle & ~0x20)` 确保 `WS_EX_TRANSPARENT` 位被清除

#### Scenario: 锁定窗口
- **WHEN** 用户触发锁定（`mIsLocked` 从 false 变为 true）
- **THEN** `SetWindowLong(hwnd, -20, extendedStyle | 0x20)` 确保 `WS_EX_TRANSPARENT` 位被设置

### Requirement: 反序列化 null 安全
系统 SHALL 对文件反序列化结果进行 null 检查，防止后续操作抛出 NullReferenceException。

#### Scenario: 队列文件损坏或为空
- **WHEN** `LoadList()` 反序列化得到 null
- **THEN** `_queue` 被初始化为空的 `List<PriorityQueueNode>`，不抛出异常

#### Scenario: 队列文件正常
- **WHEN** `LoadList()` 反序列化得到有效列表
- **THEN** `_queue` 被赋值为反序列化结果

### Requirement: ConfigChanged 参数校验
系统 SHALL 在解析配置变更消息前检查消息格式完整性。

#### Scenario: 消息包含分隔符
- **WHEN** 收到格式为 `"KEY:VALUE"` 的配置变更消息
- **THEN** `Split(':')` 后 `parts.Length >= 2`，正常处理

#### Scenario: 消息不包含分隔符
- **WHEN** 收到格式为 `"KEY"` 的配置变更消息（无冒号）
- **THEN** 跳过需要 `parts[1]` 的分支，不抛出 IndexOutOfRangeException

### Requirement: 拼写修正
系统 SHALL 使用正确的函数名 `OnReceivedRawJson`。

#### Scenario: 调用点更新
- **WHEN** `ToolsMain.OnReceivedRawMsg` 调用弹幕处理
- **THEN** 调用 `DanmuManager.GetInst().OnReceivedRawJson(rawJsonStr)`（修正拼写）

## REMOVED Requirements

### Requirement: ToolsMainIndependent 旧插件入口
**Reason**: 依赖已注释掉的 `BilibiliDM_PluginFramework.DMPlugin` 基类，引用 `DanmuManager` 中不存在的方法（`OnReceivedDanmaku`、`OnReceivedRoomCount`），包含无用的 `using` 语句，完全无法编译。
**Migration**: 使用 `ToolsMain.cs` 作为唯一入口点。

### Requirement: DanmuManager 废弃字段 medalName
**Reason**: `SetMedalName` 方法设置的 `medalName` 字段在 `IsWearingMedal` 中已被注释掉的旧逻辑使用，当前逻辑改用 `ONLY_MEDAL_ORDER` 配置项和 `fans_medal_wearing_status` 字段。
**Migration**: 无需迁移，功能已由配置系统替代。

### Requirement: ORDER_FINISH_CLICK_INTERVAL 未使用常量
**Reason**: 该常量声明后从未被引用。
**Migration**: 无需迁移。
