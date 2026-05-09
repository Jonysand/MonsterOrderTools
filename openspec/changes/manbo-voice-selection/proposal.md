## Why

Manbo TTS 引擎目前仅支持单一音色"曼波"。API 提供商已开放约 200 个免费音色，用户需要能够在 UI 中浏览并选择其他音色，同时保留"曼波"作为付费选项的标识。

## What Changes

1. **新增音色配置字段** `manboVoice`（C++）/ `MANBO_VOICE`（C#），默认值为"曼波"，持久化到 JSON 配置文件
2. **C# UI 异步获取音色列表**：程序启动后请求 `https://api.milorapart.top/apis/AIvoice/?type=list`，解析 speakers 数组
3. **ConfigWindow UI 新增音色选择 ComboBox**："曼波"置顶并标记"（付费）"，其余按 API 返回顺序排列
4. **C++ ManboTTSProvider 切换 API 端点**：
   - 音色为"曼波" → 继续使用现有 `/apis/mbAIscvip` 端点
   - 其他音色 → 使用新端点 `/apis/AIvoice?speaker=<音色>&text=<文本>`
5. **配置字段跨层同步**：C++ ConfigData ↔ C++/CLI DataBridgeWrapper ↔ C# MainConfig 双向同步
6. **错误处理**：API 获取音色列表失败时 ComboBox 仅显示"曼波（付费）"；配置中的音色不在列表中时保留原值

## Capabilities

### New Capabilities
- `manbo-voice-selection`: 支持在 UI 中选择 Manbo TTS 音色，C++ 层根据音色切换 API 端点

### Modified Capabilities
- 无

## Impact

- **C++ 层**：ConfigManager.h/.cpp, ConfigFieldRegistry.cpp, DataBridgeWrapper.h, ManboTTSProvider.cpp, ManboTTSProviderTests.cpp
- **C# 层**：Utils.cs, DataStructures.cs, ProxyClasses.cs, ToolsMain.cs, ConfigWindow.xaml, ConfigWindow.xaml.cs
- **外部依赖**：新增对 `api.milorapart.top/apis/AIvoice/?type=list` 的 GET 请求
- **配置兼容**：旧配置无 `MANBO_VOICE` 字段时默认回退到"曼波"
- **构建系统**：.vcxproj 和 .vcxproj.filters 需更新（新增/修改文件）
