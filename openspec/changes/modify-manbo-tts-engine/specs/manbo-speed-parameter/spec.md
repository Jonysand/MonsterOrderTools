## ADDED Requirements

### Requirement: Manbo TTS 请求传递 speed 参数
ManboTTSProvider SHALL 从 ConfigManager 读取 speechRate 配置，线性映射后作为 speed 参数传递给 API。

#### Scenario: speechRate 正常范围映射
- **WHEN** speechRate 配置值为 0（默认值）
- **THEN** API 请求 URL 中包含 `&speed=0`

#### Scenario: speechRate 最大值映射
- **WHEN** speechRate 配置值为 10
- **THEN** API 请求 URL 中包含 `&speed=50`

#### Scenario: speechRate 最小值映射
- **WHEN** speechRate 配置值为 -10
- **THEN** API 请求 URL 中包含 `&speed=-50`

#### Scenario: speechRate 中间值映射
- **WHEN** speechRate 配置值为 5
- **THEN** API 请求 URL 中包含 `&speed=25`

#### Scenario: speed 参数仅用于 Manbo
- **WHEN** 使用 ManboTTSProvider 发起 TTS 请求
- **THEN** TTSRequest 结构体不被修改，speed 仅在 BuildRequestUrl 内部使用
