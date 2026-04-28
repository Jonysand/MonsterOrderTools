## Why

代码库存在多个潜在 bug（XOR 位运算导致解锁失败、反序列化 null 崩溃等）和显著的性能问题（每条弹幕编译 140+ 个正则、每次入队写磁盘）。同时有大量死代码（废弃的 `ToolsMainIndependent.cs`、注释掉的旧逻辑、未使用的字段）增加了维护负担。此次整理旨在系统性修复 bug、消除性能瓶颈、清理死代码。

## What Changes

### Bug 修复
- **修复解锁逻辑**：`OrderedMonsterWindow.xaml.cs:97` 中 `extendedStyle ^ 0x20` 改为 `extendedStyle & ~0x20`，避免 XOR 翻转导致解锁后窗口仍不可点击
- **反序列化 null 防护**：`PriorityQueue.LoadList()` 中对 `DeserializeObject` 返回值做 null 检查
- **ConfigChanged 越界防护**：`ToolsMain.ConfigChanged()` 中 `Split(':')` 后检查 `parts.Length`
- **CompareTo 完善**：补充 `GuardLevel < other.GuardLevel` 返回 1 的分支，提高可读性

### 性能优化
- **Regex 预编译**：将 `DanmuManager` 和 `MonsterData` 中的字符串 pattern 替换为预编译的 `Regex` 对象，消除每条弹幕 140+ 次正则编译开销
- **Contains 改用 HashSet**：`PriorityQueue` 增加 `HashSet<string>` 维护 userId，`IsRepeatUser` 从 O(n) 降为 O(1)
- **SaveList 节流**：`Enqueue` 时不再每次都写磁盘，改为 debounce（500ms 内最多写一次）
- **消除重复加载**：去掉 `OrderedMonsterWindow.OnLoaded` 中多余的 `LoadHistoryOrder()` 调用（`PriorityQueue` 构造函数已加载）

### 代码清理
- **删除 `ToolsMainIndependent.cs`**：旧插件框架残留，无法编译，方法引用不存在
- **清理死代码**：移除 `DanmuManager` 中注释掉的旧 `IsWearingMedal` 逻辑和废弃的 `medalName` 字段
- **修正拼写**：`OnReceicedRawJson` → `OnReceivedRawJson`
- **移除未使用常量**：`ORDER_FINISH_CLICK_INTERVAL`
- **GlobalEventListener 线程安全**：为 `EventMap` 的读写加锁

## Capabilities

### New Capabilities
- `regex-optimization`: 正则表达式预编译，覆盖 DanmuManager 和 MonsterData 的所有 regex 使用点
- `queue-performance`: PriorityQueue 查找和持久化性能优化
- `bugfixes`: 跨多个文件的 bug 修复集合

### Modified Capabilities
（无现有 spec，此部分为空）

## Impact

- **受影响文件**：`DanmuManager.cs`、`MonsterData.cs`、`PriorityQueue.cs`、`OrderedMonsterWindow.xaml.cs`、`ToolsMain.cs`
- **删除文件**：`ToolsMainIndependent.cs`
- **行为变更**：解锁窗口位运算逻辑修正（bug fix，非功能变更）
- **性能变更**：弹幕处理延迟显著降低，磁盘 IO 频率降低
- **API 变更**：`DanmuManager.OnReceicedRawJson` 重命名为 `OnReceivedRawJson`（需同步更新 `ToolsMain.cs:223` 的调用点）
