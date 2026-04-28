## ADDED Requirements

### Requirement: 点赞事件解析与分发
B站 WebSocket 连接会推送 `LIVE_OPEN_PLATFORM_LIKE` 消息，系统需要正确解析其中的 uid、like_count、timestamp、msg_id 字段，并分发给 RetroactiveCheckInModule 处理。

#### Scenario: 收到标准点赞消息
- **WHEN** WebSocket 推送 `{"cmd":"LIVE_OPEN_PLATFORM_LIKE","data":{"open_id":"abc123","like_count":10,"timestamp":1777030207,"msg_id":"xxx",...}}`
- **THEN** DanmuProcessor 解析出 uid="abc123"、likeCount=10、timestamp=1777030207、msgId="xxx"
- **AND** 触发 LikeEvent 回调通知 RetroactiveCheckInModule

#### Scenario: 收到 like_count=0 的点赞消息
- **WHEN** WebSocket 推送 like_count=0 的点赞消息
- **THEN** DanmuProcessor 正常解析但不触发事件分发
- **AND** RetroactiveCheckInModule 不处理该事件

#### Scenario: 收到缺少字段的点赞消息
- **WHEN** WebSocket 推送缺少 open_id 或 like_count 字段的无效消息
- **THEN** DanmuProcessor 记录警告日志（LOG_WARNING）并丢弃该消息
- **AND** 不触发事件分发

#### Scenario: 重复 msg_id 的点赞消息
- **WHEN** 收到与之前相同 msg_id 的点赞消息（B站消息重复推送）
- **THEN** DanmuProcessor 通过LRU缓存（最近1000条）检测到重复并丢弃
- **AND** RetroactiveCheckInModule 只处理一次
