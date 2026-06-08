## MODIFIED Requirements

### Requirement: Manbo API 调用与响应解析
Manbo TTS Provider SHALL 通过 HTTP GET 请求调用新 Manbo API (`api.milorapart.top`)，传入 text、format、speed、key 参数，并解析返回的 JSON 响应获取音频 URL。

#### Scenario: API 调用成功并返回音频 URL
- **WHEN** 调用 Manbo API（https://api.milorapart.top/apis/mbAIscvip）并传入 text、format=mp3、speed、key 参数
- **THEN** 返回 HTTP 200 状态码，且响应 JSON 包含有效的 `url` 字段（非 `audio_url`）

#### Scenario: API 调用失败返回错误
- **WHEN** 调用 Manbo API 时网络错误或服务器返回错误
- **THEN** 返回 HTTP 错误状态码或非 200 的 JSON 响应，Provider 应标记为不可用并记录错误信息

#### Scenario: API 返回无效的 JSON
- **WHEN** 调用 Manbo API 返回的响应不是有效的 JSON 格式
- **THEN** 解析失败，Provider 应记录解析错误并标记为不可用

#### Scenario: API Key 缺失时请求仍正常发送
- **WHEN** manboApiKey 为空字符串
- **THEN** API 请求 URL 中 `key=` 参数值为空，由 API 服务端决定是否接受

### Requirement: UI 集成
C# 配置窗口 SHALL 提供 Manbo TTS 的引擎选择选项和独立的 API Key 配置区域。

#### Scenario: 引擎选择显示 Manbo 选项
- **WHEN** 打开配置窗口的语音设置标签页
- **THEN** TTS 引擎下拉框中包含 "Manbo" 选项

#### Scenario: Manbo API Key 输入栏可见
- **WHEN** 打开配置窗口的语音设置标签页
- **THEN** 显示独立的 Manbo 设置区域，包含 API Key 输入栏（PasswordBox），占位符文字为 "输入Manbo API Key"
