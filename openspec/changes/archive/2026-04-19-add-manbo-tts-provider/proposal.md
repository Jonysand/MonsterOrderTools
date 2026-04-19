## Why
当前 TTS 系统支持 MiniMax、小米 MiMo 和 Windows SAPI 三种引擎。用户希望增加 Manbo（曼波）TTS Provider，这是一个第三方 TTS API 服务，可以提供额外的语音合成选项，丰富语音播报的音色选择。

## What Changes
1. 在 C++ 层新增 `ManboTTSProvider` 类，实现 `ITTSProvider` 接口
2. 支持调用 Manbo API（https://api-v2.cenguigui.cn/api/speech/AiChat/）获取音频 URL，voice 参数硬编码为"曼波"
3. 支持从音频 URL 下载音频数据（WAV 格式）
4. 在 TTS Provider 工厂中注册 Manbo Provider，auto 模式优先级：manbo -> minimax -> mimo -> sapi
5. 在 TTS 引擎选择列表中增加 "Manbo" 选项
6. 实现 Provider 降级策略：当请求失败时自动降级到下一个 Provider（manbo -> minimax -> mimo -> sapi）
7. 在 UI 中实时显示当前实际使用的 TTS Provider（支持降级后的显示更新）
8. 添加单元测试覆盖新 Provider 的核心逻辑

## Capabilities
### New Capabilities
- `ManboTTSProvider`: 新的 TTS Provider，支持通过 HTTP GET 请求调用 Manbo API，解析返回的 JSON 获取 audio URL，然后下载音频数据。voice 参数硬编码为"曼波"。
- `TTSProvider降级策略`: 当当前 Provider 请求失败达到最大重试次数时，自动切换到下一个 Provider（manbo → minimax → mimo → sapi），并重置重试次数重新尝试。
- `UI实时显示当前引擎`: 在配置窗口显示当前实际使用的 TTS Provider，支持降级后的自动更新。

### Modified Capabilities
- `TTSProviderFactory`: 增加对 manbo 引擎的创建逻辑，调整 auto 模式优先级为 manbo → minimax → mimo → sapi
- `TTSManager::GetActiveEngineType`: 增加 manbo 引擎类型识别
- `TTSManager::HandleRequestFailureInternal`: 增加 Provider 降级逻辑
- `ConfigWindow`: 增加 Manbo 引擎选项和当前引擎显示 Label

## Impact
- C++ 层：`TTSProvider.h`、`TTSProviderFactory.cpp`、`TextToSpeech.cpp`、`TextToSpeech.h`、`DataBridgeExports.cpp`
- C# 层：`ConfigWindow.xaml/xaml.cs`、`NativeImports.cs`
- 项目文件：`MonsterOrderWilds.vcxproj`、`MonsterOrderWilds.vcxproj.filters`
- 新增文件：`ManboTTSProvider.cpp`
- 测试文件：`TTSProviderTests.cpp`
- 无新增外部依赖（使用现有 Network 模块）
- **不修改配置系统**（voice 字段硬编码，无需配置字段）
- **配置兼容**：老版本配置文件无需修改即可读取
