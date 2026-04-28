# MiniMax TTS UI 配置

## Why
MiniMax TTS 功能已集成但缺少 UI 配置界面。用户无法在应用中选择 MiniMax 音色和调节语速，必须通过手动修改配置实现。当前 TTS 引擎选择仅支持"自动/小米MiMo/Windows SAPI"，缺少 MiniMax 选项。

## What Changes
1. 在 TTS 引擎下拉框中添加 "MiniMax" 选项
2. 创建独立的 MiniMax 配置面板，包含：
   - 音色选择下拉框（58个选项，显示中文名称）
   - 语速滑块（范围 0.2~2）
3. 修复 MiniMaxTTSProvider 的 hex 解码 bug

## Capabilities
### New Capabilities
- **MiniMax 音色选择**: 用户可通过下拉框选择 58 种 MiniMax 音色
- **MiniMax 语速调节**: 用户可通过滑块调节语速 (0.2~2)
- **MiniMax 独立配置面板**: 当选择 MiniMax 引擎时显示专用配置界面

### Modified Capabilities
- **TTS 引擎选择**: 扩展支持 MiniMax TTS 引擎

## Impact
- 修改文件: ConfigWindow.xaml, ConfigWindow.xaml.cs, Utils.cs, DataStructures.cs, ProxyClasses.cs, ConfigManager.h, ConfigFieldRegistry.cpp, ConfigManager.cpp, MiniMaxTTSProvider.cpp
- 新增配置字段: MINIMAX_SPEED, MINIMAX_VOICE_ID
