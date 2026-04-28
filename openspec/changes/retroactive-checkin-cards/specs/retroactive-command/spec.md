## ADDED Requirements

### Requirement: 补签命令处理
用户发送"补签"弹幕时，系统验证补签卡数量并执行补签操作。

#### Scenario: 用户成功补签
- **GIVEN** 用户 "user1" 当前持有2张补签卡
- **AND** 用户最后打卡日期为 20260420，今天是 20260424（已打卡）
- **AND** 缺失日期为 20260421、20260422、20260423
- **WHEN** 用户发送弹幕 "补签"
- **THEN** 系统查询到最近缺失日期为 20260423
- **AND** 扣除1张补签卡（剩余1张）
- **AND** 在 `checkin_records` 插入 (uid="user1", checkin_date=20260423, created_at=now, username="user1")
- **AND** 调用 `CalculateContinuousDays("user1", 20260423)` 重新计算
- **AND** 更新 `user_profiles.continuous_days`
- **AND** 回复/播报："user1 已成功补签4月23日，剩余补签卡1张，连续打卡恢复为5天！"

#### Scenario: 用户没有补签卡
- **GIVEN** 用户 "user1" 当前持有0张补签卡
- **WHEN** 用户发送弹幕 "补签"
- **THEN** 回复/播报："user1，你没有补签卡哦~"
- **AND** 不执行任何数据库操作

#### Scenario: 用户无断签记录
- **GIVEN** 用户 "user1" 当前持有2张补签卡
- **AND** 用户已连续打卡到今天，无缺失日期
- **WHEN** 用户发送弹幕 "补签"
- **THEN** 回复/播报："user1，当前没有需要补签的日期"
- **AND** 不扣除补签卡

#### Scenario: 补签日期与已有记录冲突
- **GIVEN** 用户 "user1" 在 20260423 已有打卡记录
- **WHEN** 系统尝试在 20260423 插入补签记录
- **THEN** 使用 `INSERT OR REPLACE` 或 `ON CONFLICT` 处理
- **AND** 不报错，记录保持不变（或更新 timestamp）

#### Scenario: 补签后连续天数未恢复（仍有其他缺失）
- **GIVEN** 用户 "user1" 缺失 20260421、20260422、20260423 三天
- **AND** 当前持有3张补签卡
- **WHEN** 用户发送 "补签"
- **THEN** 只补最近的一天（20260423）
- **AND** 扣除1张补签卡（剩余2张）
- **AND** 回复："user1 已成功补签4月23日，剩余补签卡2张。注意：还有更早的断签日期可继续补签。"

#### Scenario: 并发补签（同时处理多个用户）
- **GIVEN** 用户A和用户B同时发送"补签"
- **WHEN** 系统并发处理两个请求
- **THEN** 使用数据库事务确保 `card_count` 原子性扣减
- **AND** 两个用户各自正确扣除1张，不会出现负数
