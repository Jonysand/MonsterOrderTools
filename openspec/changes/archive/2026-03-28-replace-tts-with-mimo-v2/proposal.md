## Why

当前项目使用Windows SAPI进行语音合成，存在以下问题：
- 语音质量有限，缺乏自然度和情感表达
- 不支持方言、角色扮演等高级语音功能
- 语音风格固定，无法根据内容动态调整
- 依赖本地Windows语音引擎，跨平台兼容性差

小米MiMo-V2-TTS提供了更高质量的语音合成能力，支持自然语言风格控制、多方言、角色扮演和歌声合成，能够显著提升直播弹幕播报的用户体验。

## What Changes

- 新增小米MiMo-V2-TTS API集成模块
- 新增编译时宏开关 `USE_MIMO_TTS`，支持完全排除小米MiMo代码
- 新增运行时配置项 `TTS_ENGINE`，支持动态切换TTS引擎
- 新增语音风格配置功能（支持自然语言描述语音风格）
- 新增多方言支持（东北话、粤语、四川话等）
- 新增角色扮演语音合成功能
- 更新配置系统以支持新的TTS参数
- 保留Windows SAPI作为备用方案，支持自动降级

## Capabilities

### New Capabilities
- `mimo-tts-integration`: 小米MiMo-V2-TTS API集成模块，负责与小米TTS服务的通信和音频播放
- `voice-style-control`: 语音风格控制功能，支持通过自然语言描述控制语音的情感、语速、音调等
- `dialect-support`: 多方言支持，包括东北话、粤语、四川话等多种方言
- `role-playing-voice`: 角色扮演语音合成，支持特定角色的声音风格

### Modified Capabilities
- `tts-configuration`: TTS配置系统，需要扩展以支持小米MiMo-V2-TTS的新参数（API Key、风格描述、方言选择等）

## Impact

- **依赖变更**: 移除Windows SAPI依赖，新增HTTP客户端依赖（用于API调用）
- **配置变更**: 需要新增小米MiMo API Key配置项
- **性能影响**: 从本地TTS转为网络API调用，可能增加延迟，但可通过异步处理和缓存优化
- **成本影响**: 需要小米MiMo API的使用费用
- **兼容性**: 需要确保网络连接可用，API服务稳定
- **代码变更**: 需要修改`TextToSpeech.h`和`TextToSpeech.cpp`，新增网络请求和音频播放逻辑
