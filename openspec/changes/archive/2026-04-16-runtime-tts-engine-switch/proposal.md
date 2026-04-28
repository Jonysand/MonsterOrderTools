# 运行时 TTS 引擎切换

## Why

当前 TTS 引擎选择在程序启动时创建，之后一直使用。即使用户在 UI 上切换引擎，已创建的 TTSProvider 不会被更新，导致配置不生效。用户必须重启程序才能切换 TTS 引擎，体验不佳。

## What Changes

1. 在 TTSManager 中添加 `RefreshTTSProvider()` 方法，支持动态重新创建 TTSProvider
2. 在 ConfigManager 中为 `ttsEngine` 字段注册 `onChanged` 回调
3. 在 TTSManager 初始化时注册配置监听器，响应 ttsEngine 变更
4. 当用户在 UI 上切换 TTS 引擎时，自动调用 `RefreshTTSProvider()` 重新创建 Provider

## Capabilities

### New Capabilities
- **运行时引擎切换**: 用户可在程序运行时切换 TTS 引擎，无需重启
- **自动生效**: 切换引擎后，新发生的弹幕/礼物等会使用新引擎

### Modified Capabilities
- **TTS 引擎配置**: 增加动态切换能力

## Impact

- 修改文件: TextToSpeech.h, TextToSpeech.cpp, ConfigFieldRegistry.cpp
- 不影响现有 API 调用流程和缓存机制