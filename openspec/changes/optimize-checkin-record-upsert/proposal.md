# Proposal: optimize-checkin-record-upsert

## Why
当前打卡记录每次打卡都执行 INSERT 操作，导致同一用户同一天多次打卡时产生冗余记录。通过改为 INSERT ... ON CONFLICT DO UPDATE 策略，减少不必要的数据库写入操作。

## What Changes
将 `checkin_records` 表的打卡记录操作从 `INSERT OR IGNORE` 改为 `INSERT ... ON CONFLICT DO UPDATE` 逻辑：
- 新用户第一次打卡：INSERT
- 老用户当日第一次打卡：UPDATE
- 老用户当日重复打卡：IGNORE

## Capabilities
### New Capabilities
- (无)

### Modified Capabilities
- `CaptainCheckInModule-打卡记录`: 优化 `ProfileManager::RecordCheckin` 和 `ProfileManager::RecordCheckinAsync` 函数中的数据库写入逻辑

## Impact
- **修改文件**: `MonsterOrderWilds/ProfileManager.cpp`
- **修改函数**: `InsertCheckinRecordWithRetry`, `RecordCheckin`, `RecordCheckinAsync`
- **影响范围**: 仅 `checkin_records` 表，不影响 `user_profiles` 表
