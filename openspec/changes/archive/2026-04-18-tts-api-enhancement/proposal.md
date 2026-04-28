## Why

MiniMax TTS API 参数需要按官方要求调整（text_normalization、audio_setting 规格化），同时 Mimo TTS 需要支持弹幕文本中的 #标签 替换为 <style> 标签以实现情感风格控制。

## What Changes

### MiniMax TTS
- voice_setting 中定死 "text_normalization": true
- audio_setting 规格化为 "sample_rate": 22050、"bitrate": 64000
- 移除 "pronunciation_dict" 字段（如存在）
- 确认音色变更即时生效（检查 TTSManager 刷新机制）

### Mimo TTS
- 弹幕文本中的成对 #标签 替换为 <style> 标签
- 示例："#唱歌#你是我心中最美的云彩" → "<style>唱歌</style>你是我心中最美的云彩"

## Capabilities

### New Capabilities
- `minimax-api-normalization`: MiniMax TTS API 参数标准化
- `mimo-hashtag-style-replacement`: Mimo TTS #标签 转 <style> 标签

### Modified Capabilities
- （无）

## Impact

- `MonsterOrderWilds/MiniMaxTTSProvider.cpp`: BuildRequestBody 修改
- `MonsterOrderWilds/XiaomiTTSProvider.cpp`: BuildRequestBody 修改（添加 #标签 处理）
- `MonsterOrderWilds/TTSProviderFactory.cpp`: 可能需要检查 provider 刷新逻辑
