## ADDED Requirements

### Requirement: 异步 TTS 播放队列

#### Scenario: 弹幕到达后立即发起 TTS 请求
- **WHEN** 弹幕消息到达系统
- **THEN** 系统立即发起 TTS HTTP 请求（不等待 Tick），最多 3 个并发请求

#### Scenario: HTTP 响应完成后进入待播放队列
- **WHEN** TTS HTTP 请求成功返回音频数据
- **THEN** 系统将音频数据通过独立线程异步写入缓存
- **AND** 缓存写入完成后，将请求状态改为 Ready 并放入待播放队列

#### Scenario: 缓存写入失败时继续播放
- **WHEN** 缓存写入失败
- **THEN** 系统仍然将请求放入待播放队列（缓存失败不影响播放）

#### Scenario: 待播放队列串行播放
- **WHEN** 待播放队列不为空且当前没有在播放
- **THEN** 系统从队列头部取出请求开始播放
- **AND** 播放完成后继续检查队列，直到队列为空

#### Scenario: 播放被打断时等待
- **WHEN** 当前正在播放
- **THEN** 系统等待当前播放完成，不立即播放下一条

#### Scenario: 请求失败重试
- **WHEN** TTS HTTP 请求失败
- **THEN** 系统根据重试计数决定是否重试
- **OR** 超过最大重试次数后标记为 Failed

### Requirement: 播放完成检测

#### Scenario: MCI 报告播放停止
- **WHEN** 音频播放器报告播放已停止
- **THEN** 系统标记当前请求为 Completed
- **AND** 清理临时文件
- **AND** 继续处理下一条

#### Scenario: 播放超时
- **WHEN** 播放时间超过配置的超时时间
- **THEN** 系统停止播放并标记为 Completed
- **AND** 清理临时文件
