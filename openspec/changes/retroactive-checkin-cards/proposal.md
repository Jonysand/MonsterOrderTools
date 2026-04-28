## Why

当前打卡系统仅支持每日打卡，用户一旦断签，连续天数就会清零。为了给积极参与互动的用户（如每日点赞）提供补签机会，激励用户持续参与直播间互动，需要引入补签卡机制。用户通过连续点赞或月度首次点赞达标可获得补签卡，用于恢复打卡连续性。

## What Changes

1. **新增 RetroactiveCheckInModule 模块**：独立处理补签卡相关逻辑
2. **点赞事件接收与解析**：在 WebSocket 消息处理中新增对 `LIVE_OPEN_PLATFORM_LIKE` 事件类型的解析
3. **每日点赞累计追踪**：为每个用户按日期累计点赞数，存储到 `user_daily_likes` 表
4. **补签卡发放逻辑**：
   - 规则1：连续7天每天点赞（like_count>0），第7天结束时发放1张
   - 规则2：每月任意一天首次点赞累计达1000，立即发放1张（每月仅1次，跨月清空）
5. **补签命令处理**：用户发送"补签"弹幕，扣除1张补签卡，在 `checkin_records` 插入最近断签日期的记录
6. **补签查询命令**：用户发送"补签查询"弹幕，通过气泡和播报回复当前持有数量和下次领取条件
7. **数据库表扩展**：新增 `user_daily_likes`、`user_like_streaks`、`retroactive_cards` 三个表
8. **连续天数重算**：补签后调用 `ProfileManager::CalculateContinuousDays` 重新计算
9. **新增单元测试**：覆盖点赞累计、补签卡发放、补签执行、查询回复等核心逻辑

## Capabilities

### New Capabilities
- `LikeEventParsing`：解析 B站 `LIVE_OPEN_PLATFORM_LIKE` WebSocket 消息，提取 uid、like_count、timestamp、msg_id
- `DailyLikeTracking`：按日期累计每个用户的点赞总数，存储到 SQLite
- `StreakRewardRule`：追踪用户连续点赞天数，达到7天时发放1张补签卡
- `MonthlyFirstRewardRule`：每月首次单日点赞达1000时发放1张补签卡，跨月自动重置
- `RetroactiveCommand`：处理"补签"弹幕命令，验证补签卡数量，执行补签并更新连续天数
- `QueryCommand`：处理"补签查询"弹幕命令，返回持有数量和下次领取进度
- `CrossMonthReset`：检测到跨月时自动清空上月首次点赞领取状态
- `MsgIdDedup`：基于msg_id的LRU去重缓存，防止B站重复推送导致重复处理

### Modified Capabilities
- `DanmuProcessor`：新增 `ParseLikeJson` 方法，将 `LIVE_OPEN_PLATFORM_LIKE` 消息解析为 LikeEvent（含msg_id）
- `DanmuProcessor`：新增点赞事件监听器机制，类似现有的 `CaptainDanmuHandler`
- `DanmuProcessor`：新增 msg_id LRU去重缓存（1000条）
- `ProfileManager`：新增 `RetroactiveCheckin` 方法，支持在指定日期插入打卡记录（标记为补签）
- `ProfileManager`：新增补签卡相关数据操作方法（Load/Save retroactive_cards、user_daily_likes、user_like_streaks）
- `ProfileManager`：新增 `AddRetroactiveCard` 原子方法，用于回滚补签卡

## Impact

- **C++ 层**：
  - 新增：`RetroactiveCheckInModule.h/cpp`、`LikeEvent.h`、单元测试文件
  - 修改：`DanmuProcessor.h/cpp`（新增点赞事件解析和分发）
  - 修改：`ProfileManager.h/cpp`（新增补签相关数据库操作）
  - 修改：`BliveManager.cpp`（新增 LIKE 消息类型处理分支）
- **C# 层**：无需修改（纯后端逻辑）
- **数据库**：`captain_profiles.db` 新增3个表
- **项目文件**：`MonsterOrderWilds.vcxproj` 和 `.filters` 添加新文件
- **无新增外部依赖**（复用现有 SQLite、nlohmann/json）
- **配置兼容**：无需新增配置字段
