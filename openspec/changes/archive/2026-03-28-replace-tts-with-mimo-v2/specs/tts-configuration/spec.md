## MODIFIED Requirements

### Requirement: TTS配置系统
系统必须提供TTS相关配置选项，支持小米MiMo-V2-TTS的参数配置。

#### Scenario: 配置API Key
- **WHEN** 用户在配置中设置小米MiMo API Key
- **THEN** 系统保存API Key，并在后续API请求中使用

#### Scenario: 配置语音风格
- **WHEN** 用户在配置中设置语音风格描述
- **THEN** 系统保存风格描述，并在语音合成时应用

#### Scenario: 配置方言
- **WHEN** 用户在配置中选择方言
- **THEN** 系统保存方言选择，并在语音合成时使用

#### Scenario: 配置角色
- **WHEN** 用户在配置中选择角色
- **THEN** 系统保存角色选择，并在语音合成时使用

#### Scenario: 配置音频格式
- **WHEN** 用户在配置中选择音频输出格式
- **THEN** 系统保存格式选择，并在API请求中指定

#### Scenario: 配置语速
- **WHEN** 用户在配置中设置语速倍率
- **THEN** 系统保存语速设置，并在API请求中传递

## ADDED Requirements

### Requirement: TTS引擎选择
系统必须支持选择TTS引擎（小米MiMo或Windows SAPI）。

#### Scenario: 选择小米MiMo引擎
- **WHEN** 用户在配置中选择小米MiMo作为TTS引擎
- **THEN** 系统使用小米MiMo-V2-TTS API进行语音合成

#### Scenario: 选择Windows SAPI引擎
- **WHEN** 用户在配置中选择Windows SAPI作为TTS引擎
- **THEN** 系统使用本地Windows SAPI进行语音合成

#### Scenario: 引擎自动降级
- **WHEN** 小米MiMo API不可用
- **THEN** 系统自动降级到Windows SAPI，并提示用户

### Requirement: 配置验证
系统必须验证TTS配置的有效性。

#### Scenario: API Key验证
- **WHEN** 用户输入API Key
- **THEN** 系统验证Key格式是否正确

#### Scenario: 配置完整性检查
- **WHEN** 系统启动时
- **THEN** 系统检查所有必需的TTS配置项是否已设置

### Requirement: 自动降级机制
系统必须在小米MiMo API不可用时自动降级到Windows SAPI。

#### Scenario: 网络连接失败触发降级
- **WHEN** 小米MiMo API网络连接失败
- **THEN** 系统自动切换到Windows SAPI进行语音合成

#### Scenario: API认证失败触发降级
- **WHEN** 小米MiMo API认证失败（API Key无效或过期）
- **THEN** 系统自动切换到Windows SAPI进行语音合成

#### Scenario: API连续失败触发降级
- **WHEN** 小米Mo API请求连续失败3次
- **THEN** 系统自动切换到Windows SAPI进行语音合成

#### Scenario: API限流触发降级
- **WHEN** 小米MiMo API返回限流错误
- **THEN** 系统自动切换到Windows SAPI进行语音合成

#### Scenario: 引擎恢复检测
- **WHEN** 系统处于降级状态且经过指定时间间隔（如5分钟）
- **THEN** 系统尝试重新连接小米MiMo API，成功则恢复使用小米MiMo

### Requirement: 降级通知
系统必须在降级发生时记录日志并通知用户。

#### Scenario: 降级日志记录
- **WHEN** 系统触发自动降级
- **THEN** 系统记录降级原因和时间到日志文件

#### Scenario: 降级用户通知
- **WHEN** 系统触发自动降级
- **THEN** 系统在界面上显示降级提示信息

#### Scenario: 恢复日志记录
- **WHEN** 系统从小米MiMo恢复成功
- **THEN** 系统记录恢复时间和状态到日志文件
