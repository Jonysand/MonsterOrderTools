# Spec: checkin-record-upsert

## ADDED Requirements

### Requirement: 打卡记录 Upsert 行为
用户打卡时，系统应根据用户历史打卡记录决定插入还是更新 checkin_records 表中的记录

#### Scenario: 新用户第一次打卡
- **WHEN** 用户 uid=123 在 checkin_date=20260417 首次打卡
- **THEN** 系统向 checkin_records 表插入新记录 (uid=123, checkin_date=20260417)

#### Scenario: 老用户在不同日期打卡
- **WHEN** 用户 uid=123 已在 20260416 打卡，现在于 20260417 打卡
- **THEN** 系统向 checkin_records 表插入新记录 (uid=123, checkin_date=20260417)

#### Scenario: 老用户在同一日期重复打卡
- **WHEN** 用户 uid=123 在 checkin_date=20260417 已有一条记录 created_at=1000
- **WHEN** 用户再次在 checkin_date=20260417 打卡，传入 created_at=2000
- **THEN** 系统执行 UPDATE，将该记录的 created_at 更新为 2000

#### Scenario: 老用户同一日期短时间内重复打卡（时间戳相同）
- **WHEN** 用户 uid=123 在 checkin_date=20260417 已有一条记录 created_at=1000
- **WHEN** 用户再次在 checkin_date=20260417 打卡，传入 created_at=1000（相同时间戳）
- **THEN** 系统忽略此次操作，不更新记录
