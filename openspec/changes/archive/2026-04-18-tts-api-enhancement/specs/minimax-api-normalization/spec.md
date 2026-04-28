## ADDED Requirements

### Requirement: MiniMax API 参数标准化

MiniMax TTS API 请求体必须符合以下规范：
- voice_setting.text_normalization 必须为 true（定死）
- audio_setting.sample_rate 必须为 22050
- audio_setting.bitrate 必须为 64000
- 不包含 pronunciation_dict 字段

#### Scenario: 正常 TTS 请求
- **WHEN** 调用 MiniMax TTS RequestTTS
- **THEN** 构建的请求体包含正确的 voice_setting 和 audio_setting

#### Scenario: 音色变更后即时生效
- **WHEN** 用户在配置中更改 minimaxVoiceId
- **THEN** 下一次 TTS 请求使用新的 voice_id
