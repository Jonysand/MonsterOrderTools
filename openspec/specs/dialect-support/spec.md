## ADDED Requirements

### Requirement: 多方言支持
系统必须支持多种方言的语音合成。

#### Scenario: 选择方言
- **WHEN** 用户在配置中选择方言（如东北话、粤语、四川话）
- **THEN** 系统使用该方言进行语音合成

#### Scenario: 方言切换
- **WHEN** 用户修改方言配置
- **THEN** 系统立即使用新方言进行后续语音合成

### Requirement: 方言参数传递
系统必须将方言选择正确传递给小米MiMo-V2-TTS API。

#### Scenario: 包含方言的API请求
- **WHEN** 系统发送语音合成请求
- **THEN** 系统在API请求中包含方言参数

#### Scenario: 方言与风格组合
- **WHEN** 用户同时设置了方言和语音风格
- **THEN** 系统将两者组合使用，生成符合要求的语音

### Requirement: 方言列表管理
系统必须维护支持的方言列表。

#### Scenario: 获取方言列表
- **WHEN** 用户查看方言选项
- **THEN** 系统显示所有支持的方言列表

#### Scenario: 方言可用性检查
- **WHEN** 系统启动时
- **THEN** 系统检查配置的方言是否仍然可用

### Requirement: 方言降级处理
系统必须在方言不支持时提供降级方案。

#### Scenario: 方言不支持
- **WHEN** 指定的方言不被API支持
- **THEN** 系统使用普通话进行合成，并提示用户方言不支持

#### Scenario: 方言合成失败
- **WHEN** 方言语音合成失败
- **THEN** 系统尝试使用普通话重新合成
