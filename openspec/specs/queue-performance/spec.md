## ADDED Requirements

### Requirement: 用户 ID 快速查找
系统 SHALL 使用 HashSet 维护队列中的用户 ID 集合，使重复用户检测为 O(1) 操作。

#### Scenario: 入队时同步更新 HashSet
- **WHEN** `PriorityQueue.Enqueue(PriorityQueueNode node)` 被调用
- **THEN** `node.UserId` 被添加到 `HashSet<string>` 中

#### Scenario: 出队时同步更新 HashSet
- **WHEN** `PriorityQueue.Dequeue(int index)` 被调用
- **THEN** 被移除节点的 `UserId` 从 `HashSet<string>` 中移除

#### Scenario: 清空时同步清空 HashSet
- **WHEN** `PriorityQueue.Clear()` 被调用
- **THEN** `HashSet<string>` 被清空

#### Scenario: 加载列表后重建 HashSet
- **WHEN** `PriorityQueue.LoadList()` 从文件反序列化队列
- **THEN** `HashSet<string>` 从反序列化结果重建，包含所有 `UserId`

#### Scenario: Contains 使用 HashSet 查找
- **WHEN** `PriorityQueue.Contains(string userId)` 被调用
- **THEN** 通过 `HashSet<string>.Contains()` 返回结果，时间复杂度 O(1)

### Requirement: 磁盘写入节流
系统 SHALL 对 `SaveList` 进行 debounce 节流，避免高频弹幕导致频繁磁盘 IO。

#### Scenario: 入队标记脏位
- **WHEN** `Enqueue` 被调用
- **THEN** 标记 `_dirty = true`，不立即写磁盘

#### Scenario: 出队标记脏位
- **WHEN** `Dequeue` 被调用
- **THEN** 标记 `_dirty = true`，不立即写磁盘

#### Scenario: 定时器检查脏位并保存
- **WHEN** 500ms 定时器触发且 `_dirty == true`
- **THEN** 执行 `SaveList()` 并重置 `_dirty = false`

#### Scenario: 关键操作后立即保存
- **WHEN** `Clear()` 被调用
- **THEN** 立即执行 `SaveList()`（清空是不可逆操作，需要持久化）
