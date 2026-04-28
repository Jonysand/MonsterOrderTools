## ADDED Requirements

### Requirement: Manbo API 调用与响应解析
Manbo TTS Provider 能够通过 HTTP GET 请求调用 Manbo API，并正确解析返回的 JSON 响应获取音频 URL。

#### Scenario: API 调用成功并返回音频 URL
- **WHEN** 调用 Manbo API（https://api-v2.cenguigui.cn/api/speech/AiChat/）并传入 text 和硬编码的 voice="曼波" 参数
- **THEN** 返回 HTTP 200 状态码，且响应 JSON 包含有效的 audio_url 字段

#### Scenario: API 调用失败返回错误
- **WHEN** 调用 Manbo API 时网络错误或服务器返回错误
- **THEN** 返回 HTTP 错误状态码或非 200 的 JSON 响应，Provider 应标记为不可用并记录错误信息

#### Scenario: API 返回无效的 JSON
- **WHEN** 调用 Manbo API 返回的响应不是有效的 JSON 格式
- **THEN** 解析失败，Provider 应记录解析错误并标记为不可用

### Requirement: 音频数据下载
从 API 返回的音频 URL 下载实际的音频数据（WAV 格式）。

#### Scenario: 音频下载成功
- **WHEN** 从 audio_url 下载音频数据
- **THEN** 返回非空的音频字节数据，且 HTTP 状态码为 200

#### Scenario: 音频 URL 无效或过期
- **WHEN** 尝试从无效或过期的 audio_url 下载音频
- **THEN** 下载失败，返回错误信息

### Requirement: Provider 生命周期管理
ManboTTSProvider 应遵循与其他 Provider 相同的生命周期管理模式。

#### Scenario: Provider 可用性检查
- **WHEN** Provider 被创建且未被标记为失败
- **THEN** IsAvailable() 返回 true

#### Scenario: Provider 失败后重置
- **WHEN** 调用 ResetAvailable() 后
- **THEN** Provider 恢复为可用状态（available_ = true）

### Requirement: TTS 引擎集成
Manbo Provider 应正确集成到现有的 TTS 引擎选择和工厂模式中。

#### Scenario: 显式选择 Manbo 引擎
- **WHEN** ttsEngine 配置为 "manbo"
- **THEN** TTSProviderFactory 创建 ManboTTSProvider 实例

#### Scenario: Auto 模式下 Manbo 优先
- **WHEN** ttsEngine 为 "auto"
- **THEN** TTSProviderFactory 优先创建 ManboTTSProvider 实例（auto 优先级：manbo -> minimax -> mimo -> sapi）

#### Scenario: Auto 模式下 minimax 有 key 时选择 minimax
- **WHEN** ttsEngine 为 "auto" 且 minimax API key 已配置
- **THEN** 优先选择 Manbo（Manbo 无需 API key，始终优先）

### Requirement: Provider 降级策略
当当前 Provider 请求失败达到最大重试次数时，应自动降级到下一个可用的 Provider。

#### Scenario: Manbo 失败降级到 MiniMax
- **WHEN** Manbo TTS 请求失败且重试次数用尽，且配置了 minimax API key
- **THEN** 自动切换到 MiniMax Provider 并重试当前请求

#### Scenario: Manbo 失败降级到 MiMo
- **WHEN** Manbo TTS 请求失败且重试次数用尽，未配置 minimax API key 但配置了 mimo API key
- **THEN** 自动切换到 MiMo Provider 并重试当前请求

#### Scenario: 所有 API Provider 失败降级到 SAPI
- **WHEN** Manbo/MiniMax/MiMo 均失败且无法继续降级
- **THEN** 最终回退到 SapiTTSProvider（本地语音合成）

#### Scenario: 降级后重置重试次数
- **WHEN** 降级到新 Provider 后
- **THEN** 重置重试计数器，使用新 Provider 重新尝试请求

### Requirement: UI 显示当前实际 TTS 引擎
配置窗口应实时显示当前实际使用的 TTS Provider（可能与配置值不同，如降级后）。

#### Scenario: 配置窗口显示当前引擎
- **WHEN** 打开配置窗口或引擎选择变更后
- **THEN** TTS 引擎下拉框旁显示蓝色 Label，显示当前实际 Provider 名称（如 "当前引擎: Manbo"）

#### Scenario: 降级后 UI 自动更新
- **WHEN** TTS Provider 因请求失败降级到下一个 Provider
- **THEN** 配置窗口中的当前引擎显示自动更新（通过 2 秒间隔轮询）

#### Scenario: 用户选择引擎后即时更新
- **WHEN** 用户在下拉框中选择新的 TTS 引擎
- **THEN** 当前引擎显示立即更新

### Requirement: UI 集成
C# 配置窗口应提供 Manbo TTS 的引擎选择选项。

#### Scenario: 引擎选择显示 Manbo 选项
- **WHEN** 打开配置窗口的语音设置标签页
- **THEN** TTS 引擎下拉框中包含 "Manbo" 选项，选择后不需要额外配置（voice 已硬编码）

### Requirement: 配置兼容性
新版本应能正常读取老版本保存的配置文件，不会出错。

#### Scenario: 读取老版本配置
- **WHEN** 加载由老版本程序保存的配置文件（不含 Manbo 相关配置）
- **THEN** 程序正常启动，不会报错或崩溃，缺失的配置使用默认值
