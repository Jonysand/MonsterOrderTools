# TTS Provider Fallback - Specification

## ADDED Requirements

### Requirement: TTS Provider Credential Fields

credentials.dat 中的 TTS 相关凭证字段需要支持 MiMo 和 MiniMax 两个独立的 provider。

#### Scenario: MiMo TTS API Key 读取成功
- **WHEN** credentials.dat 包含 `mimo_tts_api_key` 字段
- **THEN** `GetMIMO_API_KEY()` 返回该字段值

#### Scenario: MiniMax TTS API Key 读取成功
- **WHEN** credentials.dat 包含 `minimax_tts_api_key` 字段
- **THEN** `GetMINIMAX_API_KEY()` 返回该字段值

#### Scenario: 遗留字段 tts_api_key 被忽略
- **WHEN** credentials.dat 仅包含旧的 `tts_api_key` 字段
- **THEN** `GetMIMO_API_KEY()` 和 `GetMINIMAX_API_KEY()` 均返回空字符串

### Requirement: TTS Engine 自动回滚顺序

当用户选择 `ttsEngine = "auto"` 时，系统按 minimax → xiaomi → sapi 的顺序尝试可用 provider。

#### Scenario: MiniMax 可用时优先使用
- **WHEN** ttsEngine 为 "auto"，且 minimax API key 非空
- **THEN** 优先创建 MiniMaxTTSProvider

#### Scenario: MiniMax 不可用时回滚到 Xiaomi
- **WHEN** ttsEngine 为 "auto"，minimax API key 为空，xiaomi API key 非空
- **THEN** 创建 XiaomiTTSProvider

#### Scenario: 两个云端 TTS 都不可用时回滚到 SAPI
- **WHEN** ttsEngine 为 "auto"，minimax 和 xiaomi API key 均为空
- **THEN** 创建 SapiTTSProvider

#### Scenario: 用户明确选择 mimo 时只使用 MiMo
- **WHEN** ttsEngine 为 "mimo"，xiaomi API key 非空
- **THEN** 创建 XiaomiTTSProvider

#### Scenario: 用户明确选择 mimo 但 API key 为空时回滚到 SAPI
- **WHEN** ttsEngine 为 "mimo"，xiaomi API key 为空
- **THEN** 创建 SapiTTSProvider

#### Scenario: 用户明确选择 sapi 时只使用 SAPI
- **WHEN** ttsEngine 为 "sapi"
- **THEN** 创建 SapiTTSProvider

#### Scenario: 用户明确选择 minimax 时只使用 MiniMax
- **WHEN** ttsEngine 为 "minimax"，minimax API key 非空
- **THEN** 创建 MiniMaxTTSProvider

#### Scenario: 用户明确选择 minimax 但 API key 为空时回滚到 SAPI
- **WHEN** ttsEngine 为 "minimax"，minimax API key 为空
- **THEN** 创建 SapiTTSProvider