## Context

当前舰长打卡流程中，首次打卡时会调用两次 `SaveProfileToDb`：
1. `SaveProfileAsync(profile)` → 写入 profile
2. `RecordCheckinAsync(...)` → 再次写入同一 profile

这导致同一条用户记录被写入数据库两次，造成不必要的 I/O 开销。

## Goals / Non-Goals

**Goals:**
- 消除首次打卡时的重复数据库写入
- 减少 50% 的打卡数据库写入操作
- 保持数据一致性和错误处理的完整性

**Non-Goals:**
- 不改变现有数据库 schema
- 不改变异步线程模型
- 不引入新的锁竞争

## Decisions

### Decision 0: 明确涉及文件

**涉及文件**：
- `CaptainCheckInModule.cpp`：修改 `PushDanmuEvent` 中首次打卡路径的 `SaveProfileAsync` 调用
- `ProfileManager.cpp`：`RecordCheckinAsync` 需要修改（修复内存/DB 不一致、优化 move）

### Decision 1: 合并 SaveProfileToDb 调用

**选择**: 在首次打卡时，只由 `RecordCheckinAsync` 调用 `SaveProfileToDb`，移除 `SaveProfileAsync` 中的写入

**理由**:
- `RecordCheckinAsync` 已经包含了完整的 profile 数据
- `SaveProfileAsync` 主要用于非打卡场景（更新 username、关键词等）
- 避免在两个地方写入同一数据

**实现**:
```cpp
// CaptainCheckInModule::PushDanmuEvent
if (shouldCheckin) {
    // ... 更新 profile ...
    RecordCheckinAsync(...);  // 内部会 SaveProfileToDb，合并为单一写入
} else {
    SaveProfileAsync(profile);  // 只用于非打卡场景
}
```

**Capabilities 落地**：
- `checkin-atomic-save`：通过 Decision 1 实现，首次打卡时 profile 保存和 checkin 记录写入合并为单一操作（由 `RecordCheckinAsync` 统一处理）

### Decision 2: 保留各路径的异步保存

**选择**: 非打卡场景（重复打卡、非打卡消息）仍然调用 `SaveProfileAsync`

**理由**:
- 这些场景不触发 `RecordCheckinAsync`，需要单独保存
- 保持现有代码结构不变

### Decision 3: 修复内存/DB 不一致

**问题**: 当前 `RecordCheckinAsync` 中，即使 `checkin_records` INSERT 失败，`profiles_[uid] = profile` 仍会执行，导致内存已更新但 DB 未更新，程序重启后数据丢失。

**选择**: 只有当 `checkin_records` INSERT 成功时才更新内存 profile

**理由**:
- 保证内存状态与数据库状态一致
- 失败时内存保持旧值，下次重试不会覆盖已有数据

**实现**:
```cpp
// ProfileManager::RecordCheckinAsync
bool dbSuccess = true;
// ... INSERT checkin_records ...

if (dbSuccess) {
    self->SaveProfileToDb(profile);
    // 只有成功时才更新内存
    std::lock_guard<std::mutex> lock(self->profilesLock_);
    self->profiles_[uid] = profile;
}
```

### Decision 4: 使用 move 语义优化

**问题**: `RecordCheckinAsync` 中 `UserProfileData profile` 是按值拷贝，存在不必要的性能开销。

**选择**: 使用 move 语义减少拷贝

**理由**:
- profile 数据较大，move 比 copy 效率更高
- 线程执行完成后原对象销毁，move 是安全的

**实现**:
```cpp
// ProfileManager::RecordCheckinAsync
UserProfileData profile;
int64_t timestamp = GetCurrentTimestamp();
{
    std::lock_guard<std::mutex> lock(self->profilesLock_);
    auto it = self->profiles_.find(uid);
    if (it != self->profiles_.end()) {
        profile = std::move(it->second);  // move 替代 copy
    }
    // ...
}
```

## Risks / Trade-offs

| 风险 | 影响 | 缓解 |
|------|------|------|
| 异步线程竞态 | 两次写入竞争 | 异步执行顺序不确定，但最终数据正确 |
| 错误处理 | checkin_records INSERT 失败 | 内存不更新，日志记录，用户无感知（下次打卡会重试） |
| checkin_records UNIQUE 约束冲突 | 重复打卡 | SQLite 自动忽略重复 INSERT（使用 INSERT OR IGNORE），不影响连续天数计算 |
