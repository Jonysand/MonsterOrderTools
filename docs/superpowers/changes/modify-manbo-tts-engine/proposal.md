## Why

Manbo TTS 引擎的旧 API (`api-v2.cenguigui.cn`) 需要替换为新的 API 端点 (`api.milorapart.top`)，新 API 需要 API Key 鉴权且支持 speed 参数。

## What Changes

- 替换 Manbo TTS API 端点为 `api.milorapart.top/apis/mbAIscvip`
- 添加 Manbo API Key 配置字段，存储到注册表（与 idCode 同级）
- 在 ConfigWindow UI 添加 Manbo API Key 输入栏
- 支持 speed 参数：将全局 speechRate（-10~10）映射到 API speed（-50~50）
- 更新 JSON 响应解析：`url` 字段替换旧 `audio_url`

## Capabilities

### New Capabilities
- `manbo-api-key-management`: Manbo API Key 的注册表存储和 UI 输入管理
- `manbo-speed-parameter`: Manbo TTS 请求支持 speed 参数，从全局 speechRate 映射

### Modified Capabilities
- `manbo-tts-provider`: API 端点和响应格式变更

## Impact

- C++: ManboTTSProvider.cpp, ConfigManager.h/cpp, ConfigFieldRegistry.cpp, DataBridgeWrapper.h
- C#: DataStructures.cs, Utils.cs, ProxyClasses.cs, ToolsMain.cs, ConfigWindow.xaml/cs
- 注册表: HKCU\Software\MonsterOrderWilds 新增 ManboApiKey 值
