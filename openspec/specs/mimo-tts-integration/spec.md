## ADDED Requirements

### Requirement: API连接和认证
系统必须能够与小米MiMo-V2-TTS API建立连接并进行认证。

#### Scenario: 成功认证
- **WHEN** 系统启动时
- **THEN** 系统使用配置的API Key向小米MiMo API发送认证请求，并验证连接成功

#### Scenario: 认证失败
- **WHEN** API Key无效或过期
- **THEN** 系统记录错误日志，并提示用户检查API Key配置

### Requirement: 文本转语音请求
系统必须能够向小米MiMo-V2-TTS API发送文本并接收音频响应。

#### Scenario: 成功合成语音
- **WHEN** 系统接收到需要播报的文本
- **THEN** 系统向API发送请求，接收MP3格式的音频数据，并返回成功状态

#### Scenario: 文本过长
- **WHEN** 文本长度超过API限制（8K tokens）
- **THEN** 系统将文本分割为多个段落，分别合成后按顺序播放

#### Scenario: API请求失败
- **WHEN** API返回错误（如限流、服务不可用）
- **THEN** 系统记录错误日志，并尝试重试（最多3次）

### Requirement: 音频播放
系统必须能够播放从API接收的音频数据。

#### Scenario: 成功播放音频
- **WHEN** 系统接收到音频数据
- **THEN** 系统使用Windows多媒体API播放音频，并等待播放完成

#### Scenario: 音频播放失败
- **WHEN** 音频格式不支持或播放设备不可用
- **THEN** 系统记录错误日志，并跳过当前音频播放

### Requirement: 异步处理
系统必须异步处理API请求和音频播放，避免阻塞主线程。

#### Scenario: 异步API调用
- **WHEN** 系统需要合成语音
- **THEN** 系统通过 WinHTTP 异步 callback 处理API请求，不阻塞主线程

#### Scenario: 请求队列管理
- **WHEN** 多个语音合成请求同时到达
- **THEN** 系统将请求加入队列，按顺序处理，避免并发请求过多

### Requirement: 错误处理和重试
系统必须能够处理网络错误和API错误，并实现重试机制。

#### Scenario: 网络超时
- **WHEN** API请求超时（超过30秒）
- **THEN** 系统取消当前请求，并记录超时错误

#### Scenario: 重试机制
- **WHEN** API请求失败（非认证错误）
- **THEN** 系统等待指数退避时间后重试，最多重试3次

#### Scenario: 降级处理
- **WHEN** 所有重试都失败
- **THEN** 系统记录错误日志，并跳过当前语音播报
