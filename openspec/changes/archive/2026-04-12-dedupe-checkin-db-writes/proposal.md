## Why

在舰长打卡流程中，首次打卡时会调用两次 `SaveProfileToDb`：
1. `SaveProfileAsync(profile)` → 异步写入 profile
2. `RecordCheckinAsync(...)` → 再次写入同一 profile

这导致同一条用户记录被写入数据库两次，造成不必要的 I/O 开销和性能损失。

## What Changes

- 消除首次打卡时的重复数据库写入
- 将 profile 保存和打卡记录写入合并为单一操作
- 保持数据一致性和错误处理的完整性

## Capabilities

### New Capabilities
- `checkin-atomic-save`: 将 profile 保存和 checkin 记录写入合并为单一原子操作

### Modified Capabilities
- (无)

## Impact

- 涉及文件：`CaptainCheckInModule.cpp`、`ProfileManager.cpp`
- 性能影响：减少 50% 的打卡数据库写入
- 行为变更：保持外部可见行为不变

### 附加修复：现有代码问题

本次变更同时修复以下现有代码设计问题：

1. **内存/DB 不一致**：当 `RecordCheckinAsync` 中 `checkin_records` 写入失败时，不再更新内存 profile，避免程序重启后数据丢失
2. **profile 拷贝开销**：使用 move 语义优化，减少不必要的对象拷贝
