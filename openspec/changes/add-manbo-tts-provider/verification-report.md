## OpenSpec 文档验证报告

### 检查时间: 2026-04-19 10:20

### 第1轮 - 快速扫描
- [x] Proposal 存在且完整
- [x] Specs 目录存在，包含 1 个 capability
- [x] Design 存在且包含所有关键章节
- [x] Tasks 存在且包含 8 个 task group

### 第2轮 - 深度验证
#### Proposal
- [x] Why - 动机清晰
- [x] What Changes - 变化明确
- [x] Capabilities - 覆盖完整

#### Specs
- [x] manbo-tts-provider - 12 个 scenarios（新增降级策略 4 个 + UI 显示 3 个 + 配置兼容 1 个）

#### Design
- [x] Context - 背景清晰
- [x] Goals/Non-Goals - 范围明确（含降级策略和 UI 显示）
- [x] Decisions - 8 个决策点（新增降级策略和 UI 轮询）
- [x] Risks/Trade-offs - 已识别（新增降级循环和 UI 轮询风险）

#### Tasks
- [x] Scenarios → Tasks 映射完整（8 个 Task Group）
- [x] 文件路径精确
- [x] 无占位符/TBD
- [x] TDD 步骤完整

### 跨文档一致性
- [x] Proposal ↔ Specs 一致
- [x] Specs ↔ Design 一致
- [x] Design ↔ Tasks 一致

### 代码验证
- [x] Release 编译成功（0 个错误）
- [x] UnitTest 编译成功（0 个错误）
- [x] ManboTTSProvider.cpp 已创建
- [x] TTSProvider.h 已更新（添加 ManboTTSProvider 声明）
- [x] TTSProviderFactory.cpp 已更新（auto 模式 manbo 优先）
- [x] TextToSpeech.cpp/h 已更新（添加 TTSEngineType::Manbo + 降级策略 + GetCurrentProviderName）
- [x] ConfigWindow.xaml/xaml.cs 已更新（添加 Manbo 选项 + 当前引擎显示 Label）
- [x] NativeImports.cs 已更新（添加 TTSManager_GetCurrentProviderName P/Invoke）
- [x] DataBridgeExports.cpp 已更新（添加 TTSManager_GetCurrentProviderName 导出）
- [x] MonsterOrderWilds.vcxproj/filters 已更新
- [x] TTSProviderTests.cpp 已更新（添加 Manbo 单元测试 + auto 模式测试）

### 结论
**[PASS]** - Phase 3.8 验证通过，代码符合规范，编译成功。
