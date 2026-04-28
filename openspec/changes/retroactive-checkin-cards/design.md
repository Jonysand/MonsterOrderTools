## Context

当前打卡系统核心流程：
1. `BliveManager` 通过 WebSocket 接收 B站直播消息（弹幕、礼物、进入房间等）
2. `DanmuProcessor` 解析消息并分发：普通弹幕进入点怪队列，舰长弹幕触发 `CaptainCheckInModule` 学习
3. `CaptainCheckInModule` 处理"打卡"/"签到"弹幕，调用 `ProfileManager` 记录打卡并计算连续天数
4. `ProfileManager` 使用 SQLite 存储 `user_profiles` 和 `checkin_records`

当前**不处理点赞消息**（`LIVE_OPEN_PLATFORM_LIKE`），需要新增该消息类型的解析和处理。

## Goals / Non-Goals

**Goals:**
- 实现独立的 `RetroactiveCheckInModule` 模块处理补签卡逻辑
- 解析并处理 B站 `LIVE_OPEN_PLATFORM_LIKE` 点赞消息
- 按日期累计用户点赞数并持久化
- 实现连续7天点赞补签卡发放规则
- 实现每月首次点赞1000补签卡发放规则（跨月自动重置）
- 实现"补签"弹幕命令处理（验证→扣卡→补记录→重算天数）
- 实现"补签查询"弹幕命令处理（回复持有数量和领取进度）
- 添加单元测试覆盖核心逻辑
- 通过气泡和播报回复用户操作结果

**Non-Goals:**
- 不修改 `CaptainCheckInModule` 的核心打卡逻辑（打卡和补签分离）
- 不实现补签卡的UI界面（仅通过弹幕交互）
- 不支持补签指定日期（只能补最近的一个缺失日期）
- 不修改现有的 ConfigManager 配置系统（点赞阈值硬编码）
- 不实现补签卡过期机制（补签卡永久有效）
- 不修改 C# UI 层

## Decisions

1. **独立模块设计**：新建 `RetroactiveCheckInModule` 而非集成到 `CaptainCheckInModule`。打卡和补签是两个独立的业务领域，分离后代码更清晰，便于独立测试和维护。

2. **直接操作数据库**：`RetroactiveCheckInModule` 直接通过 `ProfileManager` 操作数据库，不通过 `CaptainCheckInModule` 中转。减少模块间耦合，补签逻辑不依赖打卡模块的运行状态。

3. **点赞事件通过 DanmuProcessor 分发**：复用现有的 `CaptainDanmuHandler` 模式，新增 `LikeEventHandler` 监听器机制。`DanmuProcessor` 解析点赞消息后通知所有监听器。

4. **补签卡阈值硬编码**：连续7天、点赞1000、like_count>0 等阈值直接硬编码在代码中，不放入配置系统。规则简单且稳定，无需用户配置。

5. **每月按自然月重置**：`monthly_first_claimed` 存储日期（YYYYMMDD），通过 `date/100` 比较月份。跨月时自动重置为0。

6. **每日点赞独立计算**：每天的点赞累计是独立的。今天点了800未达1000，明天需要重新从0开始累计到1000，昨天的800不计入今天。

6. **一次只补最近的一个缺失日期**：用户有多个缺失日期时，每次"补签"命令只补最近的一天。引导用户多次补签，增加互动。

7. **SQLite 事务保证原子性**：补签操作（查卡→扣卡→插记录→算天数）封装在事务中，避免并发导致卡片数量错误。

8. **回复格式统一**：所有回复统一使用 "用户名 + 状态 + 详情" 的格式，通过 `g_aiReplyCallback` 发送气泡消息，同时调用 TTS 播报。

9. **点赞累计按timestamp日期归属**：使用点赞事件中的 timestamp 计算 YYYYMMDD，而不是系统当前时间。避免消息延迟导致日期归属错误。

10. **单元测试覆盖核心场景**：为点赞累计、奖励发放、补签执行、查询回复分别编写独立测试函数，使用内存数据库隔离测试数据。

11. **用户标识使用 open_id**：所有补签卡相关表（`user_daily_likes`、`user_like_streaks`、`retroactive_cards`）使用 B站 open_id（TEXT）作为唯一用户标识，与弹幕类消息的 numeric uid 区分。点赞消息解析时优先取 `open_id` 字段。

12. **触发词精确匹配**：用户弹幕命令使用 `wstring == word` 精确全等匹配，而非子串匹配，避免误触。`SetTriggerWords` 使用 `;` 分隔补签词和查询词两部分，各自独立精确匹配。

13. **连续奖励每7天周期发放**：CheckRule1 使用 `currentStreak % STREAK_DAYS_REQUIRED == 0` 确保每满7天（第7、14、21天...）发放一次，而非每个新日期都检查。`streakRewardIssued` 防止同一天重复发放。

14. **点赞路径与命令路径共享锁**：`PushLikeEvent` 和 `PushDanmuEvent` 共享 `likeLock_` 互斥锁，确保并发修改 `retroactive_cards` 时线程安全。`DanmuProcessor` 中的 `likeEventListeners_` 由独立的 `likeListenersMutex_` 保护。

15. **msg_id LRU去重缓存**：`DanmuProcessor` 维护最近1000条 `msg_id` 的LRU缓存（`deque + unordered_set`），防止B站重复推送导致重复处理。缓存满时淘汰最旧的msg_id。

16. **补签卡原子回滚**：`ProfileManager::AddRetroactiveCard()` 使用 `BEGIN IMMEDIATE` 事务原子性增加卡片数量，确保补签失败时的回滚操作线程安全。

17. **补签后连续天数基于补签日期计算**：`HandleRetroactiveCommand` 中调用 `CalculateContinuousDays(uid, missingDate)` 而非 `currentDate`，确保连续天数反映补签后的实际状态。

## Risks / Trade-offs

1. **点赞消息高频风险**：B站点赞消息可能比弹幕更频繁，可能导致数据库写入压力。缓解：使用异步批量写入或内存缓存+定期刷盘。

2. **连续点赞计算复杂度**：需要每日查询前N天的点赞记录来判断连续性。缓解：使用 `user_like_streaks` 缓存当前连续天数，避免每次全表扫描。

3. **补签后连续天数计算不准确**：`CalculateContinuousDays` 基于 `checkin_records` 表计算，补签插入记录后自动生效，无需额外逻辑。

4. **并发补签扣卡风险**：多个用户同时补签可能导致 race condition。缓解：SQLite 事务 + `BEGIN IMMEDIATE` 锁定。

5. **直播跨天边界问题**：timestamp 跨0点的点赞消息归属到新的一天。缓解：使用 timestamp 而非系统时间计算日期。

6. **消息去重不完整**：如果 B站重复推送相同 msg_id 的点赞消息，DanmuProcessor 需要维护去重缓存。缓解：在 DanmuProcessor 层维护最近1000条 msg_id 的LRU内存缓存（`deque + unordered_set`），已实现。

7. **like_count 聚合风险**：用户连续快速点赞时，B站可能聚合发送（like_count>1）。缓解：累加处理，不丢失计数。

8. **点赞与补签命令并发竞态**：点赞奖励发放（CheckRule1/2）和补签命令执行（DeductRetroactiveCard）同时操作 retroactive_cards。缓解：`likeLock_` 序列化两个路径的卡片读写。`DeductRetroactiveCard` 内部使用 `BEGIN IMMEDIATE` 事务。

9. **DanmuProcessor 监听器生命周期**：`Init()` 注册的 lambda 监听器在模块 `Destroy()` 后若被调用会通过 `Inst()` 重建单例。缓解：析构函数中调用 `DanmuProcessor::ClearLikeEventListeners()` 清理监听器，且检查 `GetDestroyingFlag()` 避免重复重建。

## Data Model Details

### user_daily_likes
| 字段 | 类型 | 说明 |
|------|------|------|
| uid | TEXT PK | 用户唯一标识 |
| like_date | INTEGER PK | 日期 YYYYMMDD |
| total_likes | INTEGER | 当日累计点赞数 |

索引：`PRIMARY KEY (uid, like_date)`

### user_like_streaks
| 字段 | 类型 | 说明 |
|------|------|------|
| uid | TEXT PK | 用户唯一标识 |
| current_streak | INTEGER | 当前连续点赞天数 |
| last_like_date | INTEGER | 最后点赞日期 |
| streak_reward_issued | INTEGER | 最近一次发放连续奖励的日期 |

### retroactive_cards
| 字段 | 类型 | 说明 |
|------|------|------|
| uid | TEXT PK | 用户唯一标识 |
| card_count | INTEGER | 持有补签卡数量 |
| total_earned | INTEGER | 累计获得数量 |
| monthly_first_claimed | INTEGER | 本月首次领取日期（0表示未领取） |
| last_earned_date | INTEGER | 最后获得日期 |

## Architecture Details

### 模块交互时序图（点赞事件）
```
BliveManager -> DanmuProcessor: 收到 LIVE_OPEN_PLATFORM_LIKE JSON
DanmuProcessor -> DanmuProcessor: ParseLikeJson() 解析
DanmuProcessor -> RetroactiveCheckInModule: PushLikeEvent(likeEvent)
RetroactiveCheckInModule -> RetroactiveCheckInModule: ProcessLike()
RetroactiveCheckInModule -> ProfileManager: AddDailyLike(uid, date, count)
ProfileManager -> SQLite: INSERT/UPDATE user_daily_likes
RetroactiveCheckInModule -> RetroactiveCheckInModule: CheckRule1_StreakReward()
RetroactiveCheckInModule -> ProfileManager: Load/Save user_like_streaks
RetroactiveCheckInModule -> ProfileManager: Load/Save retroactive_cards
RetroactiveCheckInModule -> (可选): SendReply("恭喜...获得补签卡")
```

### 模块交互时序图（补签命令）
```
DanmuProcessor -> RetroactiveCheckInModule: PushDanmuEvent(danmuEvent)
RetroactiveCheckInModule -> RetroactiveCheckInModule: HandleRetroactiveCommand()
RetroactiveCheckInModule -> ProfileManager: GetRetroactiveCardCount(uid)
RetroactiveCheckInModule -> ProfileManager: FindLastMissingDate(uid, today)
RetroactiveCheckInModule -> ProfileManager: RetroactiveCheckin(uid, missingDate)
ProfileManager -> SQLite: BEGIN TRANSACTION
ProfileManager -> SQLite: UPDATE retroactive_cards SET card_count -= 1
ProfileManager -> SQLite: INSERT INTO checkin_records (uid, date, ...)
ProfileManager -> SQLite: COMMIT
RetroactiveCheckInModule -> ProfileManager: CalculateContinuousDays(uid, missingDate)
RetroactiveCheckInModule -> (可选): SendReply("已成功补签...")
```

## Testing Strategy

- **单元测试文件**：`RetroactiveCheckInModuleTests.cpp`
- **测试数据库**：使用临时 SQLite 数据库（`:memory:` 或临时文件）
- **覆盖场景**：
  1. 点赞累计计算（首次、累加、跨日）
  2. 连续7天奖励（正常、中断、不重复、14天）
  3. 月首赞奖励（首次、重复、跨月、直接超1000）
  4. 跨月重置（跨月时重置、新月份首次触发）
  5. 补签执行（成功、无卡、无缺失、冲突、多缺失）
  6. 查询回复（有卡、无卡、本月已领、跨月）
  7. 并发安全（多线程扣卡验证）
