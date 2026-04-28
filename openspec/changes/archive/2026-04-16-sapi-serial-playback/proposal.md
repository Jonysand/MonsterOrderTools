# SAPI Serial Playback

## Why

当前 TTS 播放架构中，SAPI 引擎使用独立线程同步播放，不经过 `audioPlayer` 队列系统。当 SAPI 作为降级方案或用户直接配置使用 SAPI 时，TTS 播放会与其他 TTS 流（如 MiniMax/MiMo）同时播放，破坏了串行播放的保证。用户报告重复打卡时出现 UI 卡顿和播放不完整的问题。

## What Changes

- 重构 SAPI 播放路径，使其经过 `audioPlayer` 队列系统
- 确保所有 TTS 引擎（MiniMax/MiMo/SAPI）的播放都是串行的
- SAPI 播放完成后再处理下一个 TTS 请求

## Capabilities

### New Capabilities
- `sapi-audio-playback`: SAPI TTS 通过 audioPlayer 队列系统串行播放

### Modified Capabilities
- `tts-provider`: 当前 SAPI 同步播放逻辑需要修改为异步队列模式

## Impact

- `TextToSpeech.cpp`: 修改 `ProcessPendingRequest` 中 SAPI 处理逻辑
- `audioPlayer`: 可能需要支持 SAPI 音频格式（如果尚未支持）

## Non-goals

- 不改变 MiniMax/MiMo 的现有播放流程
- 不修改 TTS 引擎选择和降级逻辑
- 不改变气泡显示逻辑