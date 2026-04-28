## ADDED Requirements

### Requirement: MiniMax TTS 引擎选择
MiniMax TTS 引擎可通过下拉框选择，所有 TTS 配置面板（MiniMax、MiMo、SAPI）始终同时显示在界面上。

#### Scenario: 用户选择 MiniMax 引擎
- **WHEN** 用户在 TTS 引擎下拉框中选择 "MiniMax"
- **THEN** TTS_ENGINE 配置项更新为 "minimax"

### Requirement: MiniMax 音色选择
用户可以通过下拉框选择 58 种 MiniMax 音色，下拉框显示中文名称，对应 MiniMax API 的 voice_id。

#### Scenario: 默认音色
- **WHEN** 用户首次使用 MiniMax TTS
- **THEN** 默认选择 "甜美女性音色" (female-tianmei)

#### Scenario: 音色选择并保存
- **WHEN** 用户选择音色并保存设置
- **THEN** 配置项 MINIMAX_VOICE_ID 更新为对应的 voice_id

#### Scenario: 音色选择变更即时生效
- **WHEN** 用户切换音色选择
- **THEN** 变更通过 GlobalEventListener 立即传递到配置层

### Requirement: MiniMax 语速调节
用户可以通过滑块调节 MiniMax TTS 的语速，范围 1.0~2。

#### Scenario: 滑块范围验证
- **WHEN** 滑块值超出 0.2~2 范围
- **THEN** 滑块自动吸附到最近的有效值

#### Scenario: 默认语速
- **WHEN** 用户首次使用 MiniMax TTS
- **THEN** 默认语速为 1.5

#### Scenario: 语速调节即时生效
- **WHEN** 用户拖动滑块
- **THEN** 变更通过 GlobalEventListener 立即传递到配置层

### Requirement: MiniMax API Hex 解码修复
MiniMax TTS API 返回 hex 编码的音频数据，必须使用 hex 解码而非 base64 解码。

#### Scenario: 成功解析 hex 编码音频
- **WHEN** MiniMax API 返回 hex 编码的音频数据
- **THEN** 系统使用 HexToBytes 解码，正确还原音频数据

#### Scenario: 解析失败
- **WHEN** API 返回无效数据或错误
- **THEN** 系统记录错误信息到 errorMsg 字段

### Requirement: MiniMax 配置持久化
MiniMax 的 speed 和 voice_id 配置必须持久化到配置文件。

#### Scenario: 保存配置
- **WHEN** 用户点击保存设置
- **THEN** MINIMAX_SPEED 和 MINIMAX_VOICE_ID 保存到 JSON 配置

#### Scenario: 加载配置
- **WHEN** 应用启动并加载配置
- **THEN** MiniMax 配置项正确恢复到 UI
