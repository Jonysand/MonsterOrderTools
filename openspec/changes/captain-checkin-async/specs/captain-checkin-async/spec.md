# 舰长签到异步处理规格

## ADDED Requirements

### Requirement: AI 回复异步生成

系统 SHALL 支持异步生成 AI 回复，UI 线程不被阻塞。

#### Scenario: 签到时 AI 回复异步生成
- **WHEN** 舰长发送打卡弹幕
- **THEN** 系统立即返回，不等待 AI API 响应
- **AND** AI 回复通过回调返回

#### Scenario: AI API 调用失败时使用固定回复
- **WHEN** AI API 调用超时或返回错误
- **THEN** 系统使用固定模板生成回复
- **AND** 不阻塞弹幕处理流程

### Requirement: 锁粒度优化

系统 SHALL 只在访问共享数据时短暂持锁，AI 调用和数据库操作在锁外执行。

#### Scenario: 多个舰长同时签到
- **WHEN** 多个舰长在 1 秒内发送打卡弹幕
- **THEN** 每个签到请求独立处理，不串行等待
- **AND** UI 线程保持响应

#### Scenario: profilesLock 只在数据访问时持有
- **WHEN** PushDanmuEvent 处理签到消息
- **THEN** profilesLock 只在 LoadProfileFromDb/SaveProfileToDb 期间持有
- **AND** AI API 调用在锁外执行

### Requirement: 数据库操作异步化

系统 SHALL 支持数据库操作在后台线程执行。

#### Scenario: 数据库写入不阻塞主流程
- **WHEN** 需要保存用户画像或打卡记录
- **THEN** 数据库写入在后台线程执行
- **AND** 主流程立即继续处理下一条弹幕

### Requirement: 高并发场景 UI 响应

系统 SHALL 在高并发场景（每秒 100+ 弹幕）下保持 UI 响应。

#### Scenario: 高流量时 UI 不阻塞
- **WHEN** 系统在 1 秒内收到 100 条弹幕
- **THEN** UI 帧率保持在 60fps 以上
- **AND** 所有弹幕都被正确处理

#### Scenario: 签到音频播放不丢失
- **WHEN** 多个舰长在短时间内连续签到
- **THEN** 每个签到的 AI 回复 TTS 都被播放
- **AND** 不会因为并发而丢失音频

#### Scenario: TTS 队列支持多并发
- **WHEN** 同时有多个签到请求需要 TTS 播放
- **THEN** 系统最多同时处理 3 个 TTS 请求
- **AND** 超出上限的请求在队列中等待

### Requirement: 风险缓解验证

系统 SHALL 验证异步操作的风险缓解措施有效。

#### Scenario: 数据库并发写入安全
- **WHEN** 多个签到请求同时需要保存用户画像
- **THEN** 系统使用 SQLite WAL 模式处理并发
- **AND** 不会因为并发写入导致数据库锁定

#### Scenario: 线程生命周期管理
- **WHEN** AI 回复生成完成并触发回调后
- **THEN** 后台线程正确结束，无内存泄漏
- **AND** 回调中的 TTS 操作在主线程执行

### Requirement: 现有功能边界保持

系统 SHALL 不改变明确声明不修改的功能。

#### Scenario: TTS 引擎选择逻辑不变
- **WHEN** 配置的 TTS 引擎为 mimo/sapi/auto
- **THEN** 签到 AI 回复使用与弹幕 TTS 相同的引擎选择逻辑
- **AND** 引擎回退机制保持不变
