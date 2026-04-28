# Design: optimize-checkin-record-upsert

## Context
当前 `checkin_records` 表使用 `INSERT OR IGNORE` 策略处理打卡记录，导致每次打卡都执行 INSERT 操作。同一天重复打卡时由于 UNIQUE 约束被忽略，但数据库仍然执行了写入操作。

## Goals / Non-Goals
**Goals:**
- 减少 `checkin_records` 表的数据库写入操作
- 新用户第一次打卡时 INSERT
- 老用户当日第一次打卡时 UPDATE 时间戳
- 老用户当日重复打卡时 IGNORE（不更新）

**Non-Goals:**
- 不修改 `user_profiles` 表的操作逻辑
- 不改变 `checkin_records` 表的表结构

## Decisions
### 1. 使用 INSERT ... ON CONFLICT DO UPDATE 替代 INSERT OR IGNORE

**SQL 变更：**
```sql
-- 当前（INSERT OR IGNORE）
INSERT OR IGNORE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)

-- 改为（INSERT ... ON CONFLICT DO UPDATE）
INSERT INTO checkin_records (uid, checkin_date, created_at, username)
VALUES (?, ?, ?, ?)
ON CONFLICT(uid, checkin_date) DO UPDATE SET
    created_at = excluded.created_at,
    username = excluded.username
WHERE checkin_records.created_at != excluded.created_at
```

### 2. WHERE 条件确保时间戳相同时 IGNORE
当 `created_at` 与已存在记录相同时，WHERE 条件不满足，实际效果等同于 IGNORE。

### 3. 修改函数
- `InsertCheckinRecordWithRetry()`: 修改 SQL 语句和逻辑
- `RecordCheckin()`: 调用新的 upsert 逻辑
- `RecordCheckinAsync()`: 调用新的 upsert 逻辑

## Risks / Trade-offs
- **风险**: SQLite ON CONFLICT 语法兼容性（SQLite 3.24.0+ 支持）
- **权衡**: 当前项目使用的 SQLite 版本已支持该语法
- **影响**: 不影响 `user_profiles` 表，只修改 `checkin_records` 相关操作
