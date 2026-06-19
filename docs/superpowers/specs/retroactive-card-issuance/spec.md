# retroactive-card-issuance Specification

## Purpose
TBD - created by archiving change retroactive-checkin-cards. Update Purpose after archive.
## Requirements
### Requirement: 每日点赞累计与存储
系统需要按日期（YYYYMMDD）累计每个用户的点赞总数，并持久化到数据库。

#### Scenario: 用户首次点赞
- **WHEN** 用户 "user1" 在 20260424 收到 like_count=5 的点赞事件
- **THEN** 数据库 `user_daily_likes` 中插入记录 (uid="user1", like_date=20260424, total_likes=5)

#### Scenario: 用户同日内多次点赞
- **GIVEN** 用户 "user1" 在 20260424 已有 total_likes=20
- **WHEN** 再次收到 like_count=15 的点赞事件
- **THEN** 记录更新为 total_likes=35

#### Scenario: 跨日点赞
- **GIVEN** 用户 "user1" 在 20260424 已有 total_likes=100
- **WHEN** 在 20260425 收到 like_count=10 的点赞事件（timestamp 对应新日期）
- **THEN** 插入新记录 (uid="user1", like_date=20260425, total_likes=10)
- **AND** 20260424 的记录保持不变

### Requirement: 补签卡发放 - 连续7天规则
用户连续7天每天都有点赞（like_count>0），在第7天结束时发放1张补签卡。

#### Scenario: 用户连续点赞7天
- **GIVEN** 用户 "user1" 在 20260418~20260423 每天都有点赞记录
- **WHEN** 20260424 收到点赞事件（第7天）
- **THEN** 检查 `user_like_streaks.current_streak` 达到7
- **AND** `streak_reward_issued` 不是 20260424
- **AND** `retroactive_cards.card_count` 增加1
- **AND** `streak_reward_issued` 更新为 20260424
- **AND** 系统播报/回复："恭喜 user1 连续7天点赞，获得1张补签卡！"

#### Scenario: 连续点赞中断
- **GIVEN** 用户 "user1" 已连续点赞6天（20260418~20260423）
- **WHEN** 20260425 才收到下一个点赞（跳过了20260424）
- **THEN** `current_streak` 重置为1（以20260425为第1天）
- **AND** 不发放补签卡

#### Scenario: 连续7天奖励不重复发放
- **GIVEN** 用户 "user1" 已连续点赞7天并在20260424获得过奖励
- **WHEN** 20260425 继续点赞（第8天）
- **THEN** 检查 `streak_reward_issued` 为 20260424，与当前日期不同
- **AND** 需要重新计算是否新的7天周期已完成（第8天不满足新周期）
- **AND** 不重复发放奖励

#### Scenario: 连续14天获得第二张
- **GIVEN** 用户 "user1" 已连续点赞13天并在第7天（20260424）获得过奖励
- **WHEN** 20260501（第14天，假设期间每天点赞）收到点赞事件
- **THEN** 检查 `current_streak` 达到14，且 `streak_reward_issued` 为 20260424（不是当前周期）
- **AND** 发放第2张补签卡
- **AND** 更新 `streak_reward_issued` 为 20260501

### Requirement: 补签卡发放 - 每周首次点赞30规则
用户每周任意一天内首次点赞累计达到30，立即发放1张补签卡。每天的点赞累计独立计算，跨周需重新点满30。

#### Scenario: 用户本周首次点赞达30
- **GIVEN** 用户 "user1" 在 20260424 当前 total_likes=20
- **WHEN** 收到 like_count=15 的点赞事件
- **THEN** total_likes 更新为 35
- **AND** 检查 `weekly_first_claimed` 不在本周
- **AND** `retroactive_cards.card_count` 增加1
- **AND** `weekly_first_claimed` 更新为 20260424
- **AND** 系统播报/回复："恭喜 user1 今日点赞突破30，获得1张补签卡！"

#### Scenario: 用户同周内第二次达到30
- **GIVEN** 用户 "user1" 在 20260424 已领取过首次30奖励（weekly_first_claimed=20260424）
- **WHEN** 20260425 点赞累计达到50
- **THEN** 检查 `weekly_first_claimed` 在本周
- **AND** 不发放补签卡

#### Scenario: 用户今天未达30，次日重新计算
- **GIVEN** 用户 "user1" 在 20260424 点赞累计为20（未满30）
- **WHEN** 20260425 开始新的直播日，收到 like_count=10
- **THEN** 20260425 的 total_likes=10（不是30）
- **AND** 需要继续累计到30才能在20260425获得奖励
- **AND** 20260424 的20不计入20260425

#### Scenario: 跨周后重新获得资格
- **GIVEN** 用户 "user1" 在上周已领取过首次30奖励
- **WHEN** 20260501 点赞累计达到30
- **THEN** 检查 `weekly_first_claimed` 的周数与本周不同
- **AND** 清空上周领取状态（重置 `weekly_first_claimed=0`）
- **AND** 发放1张补签卡
- **AND** 更新 `weekly_first_claimed` 为 20260501

#### Scenario: 用户首次点赞直接超过30
- **GIVEN** 用户 "user1" 是新用户，无历史记录
- **WHEN** 第一次点赞 like_count=50
- **THEN** total_likes=50，立即触发首次30奖励
- **AND** 发放1张补签卡

### Requirement: 跨周自动重置
系统需要检测跨周并自动重置每周首次点赞的领取状态。

#### Scenario: 跨周时首次点赞触发重置
- **GIVEN** 用户 "user1" 的 `weekly_first_claimed=20260424`
- **WHEN** 20260501 收到第一个点赞事件
- **THEN** 比较周数发现跨周
- **AND** 重置 `weekly_first_claimed=0`
- **AND** 正常处理该点赞事件（可继续累计和判断奖励）

