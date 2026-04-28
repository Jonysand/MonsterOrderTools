## 1. Bug 修复

- [x] 1.1 修复 `OrderedMonsterWindow.xaml.cs:97` 解锁逻辑：`extendedStyle ^ 0x20` → `extendedStyle & ~0x20`
- [x] 1.2 `PriorityQueue.LoadList()` 反序列化 null 检查：若结果为 null 则初始化为空列表
- [x] 1.3 `ToolsMain.ConfigChanged()` 增加 `parts.Length` 校验，防止 `parts[1]` 越界
- [x] 1.4 修正 `DanmuManager.OnReceicedRawJson` → `OnReceivedRawJson` 拼写，同步更新 `ToolsMain.cs` 调用点

## 2. Regex 预编译

- [x] 2.1 `DanmuManager`：将 `order_monster_patterns` 改为 `Regex[]`，构造函数中用 `RegexOptions.Compiled` 初始化
- [x] 2.2 `DanmuManager`：将 `priority_patterns_withoutOrder` 改为 `Regex[]`，同上
- [x] 2.3 `DanmuManager.NormalizeMonsterName()`：将 3 个历战前缀 pattern 改为预编译 `Regex` 字段
- [x] 2.4 `MonsterData`：`ORDERRABLE_MONSTERS` 改为 `List<KeyValuePair<Regex, string>>`，`LoadJsonData` 中预编译所有昵称 pattern
- [x] 2.5 `MonsterData.GetMatchedMonsterName()`：使用预编译的 Regex 列表进行匹配

## 3. PriorityQueue 性能优化

- [x] 3.1 `PriorityQueue` 增加 `HashSet<string> _userIds` 字段
- [x] 3.2 `Enqueue` 方法中同步添加 userId 到 HashSet
- [x] 3.3 `Dequeue` 方法中同步从 HashSet 移除 userId
- [x] 3.4 `Clear` 方法中同步清空 HashSet
- [x] 3.5 `LoadList` 反序列化后重建 HashSet
- [x] 3.6 `Contains` 方法改为 `HashSet<string>.Contains()` 实现

## 4. SaveList 节流

- [x] 4.1 `PriorityQueue` 增加 `_dirty` 标志位和 `DispatcherTimer`
- [x] 4.2 `Enqueue` 标记 `_dirty = true`，移除直接 `SaveList()` 调用
- [x] 4.3 `Dequeue` 标记 `_dirty = true`，移除直接 `SaveList()` 调用
- [x] 4.4 定时器回调中检查 `_dirty`，为 true 时调用 `SaveList()` 并重置
- [x] 4.5 `Clear` 保持立即 `SaveList()`（清空操作需确保持久化）

## 5. 代码清理

- [x] 5.1 删除 `ToolsMainIndependent.cs`
- [x] 5.2 移除 `DanmuManager` 中注释掉的旧 `IsWearingMedal` 逻辑（:208-212）
- [x] 5.3 移除 `DanmuManager` 中废弃的 `medalName` 字段和 `SetMedalName` 方法
- [x] 5.4 移除 `OrderedMonsterWindow` 中未使用的 `ORDER_FINISH_CLICK_INTERVAL` 常量
- [x] 5.5 移除 `OrderedMonsterWindow.OnLoaded` 中多余的 `LoadHistoryOrder()` 调用
- [x] 5.6 `GlobalEventListener` 的 `EventMap` 读写加 `lock` 保护
