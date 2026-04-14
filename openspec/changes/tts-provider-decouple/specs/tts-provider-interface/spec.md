## MODIFIED Requirements

### Requirement: TTS接口抽象
TTSManager 通过 ITTSProvider 接口调用 TTS 提供者，實現解耦。

#### Scenario: 通过接口调用 TTS
- **WHEN** TTSManager 需要发起语音合成请求
- **THEN** 调用 `ttsProvider->RequestTTS(request, callback)`，不直接持有 MimoTTSClient

#### Scenario: 接口所有特化字段
- **WHEN** 需要使用 MiMo 特有的 style 参数
- **THEN** style 字段存储在 TTSRequest 中，由 XiaomiTTSProvider 处理其序列化

#### Scenario: 请求结构体清理
- **WHEN** AsyncTTSRequest 中存在已删除配置的残留字段
- **THEN** 删除 dialect、role 字段，保持结构体整洁

### Requirement: TTSProvider 接口完整性
ITTSProvider 接口必须满足 TTSManager 的所有调用需求。

#### Scenario: 检查提供者可用性
- **WHEN** TTSManager 调用 `IsAvailable()`
- **THEN** 返回当前 TTS 提供者是否可用

#### Scenario: 获取错误信息
- **WHEN** TTS 请求失败
- **THEN** TTSManager 通过 `GetLastError()` 获取错误描述

#### Scenario: 异步请求处理
- **WHEN** TTSManager 调用 `RequestTTS()`
- **THEN** 提供者通过 callback 异步返回结果
