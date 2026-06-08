## ADDED Requirements

### Requirement: Manbo API Key 注册表存储
系统 SHALL 将 Manbo API Key 存储到 Windows 注册表 `HKCU\Software\MonsterOrderWilds\ManboApiKey`，与 idCode 同级。

#### Scenario: 加载配置时从注册表读取 API Key
- **WHEN** ConfigManager::LoadConfig() 被调用
- **THEN** 从注册表读取 ManboApiKey 值，赋值到 ConfigData.manboApiKey 字段

#### Scenario: 保存配置时将 API Key 写入注册表
- **WHEN** ConfigManager::SaveConfig() 被调用
- **THEN** 将 ConfigData.manboApiKey 写入注册表 `HKCU\Software\MonsterOrderWilds\ManboApiKey`

#### Scenario: 设置 API Key 时通知变更
- **WHEN** 调用 ConfigManager::SetManboApiKey(value)
- **THEN** 更新 config_.manboApiKey，标记 dirty，并触发 ConfigChanged 通知

### Requirement: Manbo API Key 配置字段注册
manboApiKey 字段 SHALL 在 ConfigFieldRegistry（C++）和 Utils.ConfigFieldRegistry（C#）中注册，支持 DataBridge 跨层访问。

#### Scenario: C++ 字段注册
- **WHEN** ConfigFieldRegistry::RegisterAll() 执行
- **THEN** 注册名为 "manboApiKey" 的 String 类型字段，offset 指向 ConfigData.manboApiKey

#### Scenario: C# 字段注册
- **WHEN** Utils.ConfigFieldRegistry 静态构造函数执行
- **THEN** 注册名为 "manboApiKey" 的字段，通过 NativeImports 读写 C++ 层

#### Scenario: DataBridge 跨层传输
- **WHEN** ConfigProxy::Refresh() 和 ConfigProxy::Apply() 被调用
- **THEN** manboApiKey 的值正确在 C++ ConfigData 和 C# ConfigProxy 之间同步

### Requirement: UI 提供 Manbo API Key 输入栏
配置窗口 SHALL 在 TTS 设置页面提供独立的 Manbo 设置区域，包含 API Key 输入栏。

#### Scenario: 配置窗口加载时显示 API Key
- **WHEN** ConfigWindow.FillConfig() 被调用
- **THEN** ManboApiKeyTextBox 显示从配置读取的 API Key 值

#### Scenario: 用户输入 API Key 并保存
- **WHEN** 用户在 ManboApiKeyTextBox 中输入 API Key 后点击保存
- **THEN** API Key 通过 ConfigChanged 写入 C++ 配置层并保存到注册表
