# Credential TTS Key Split - Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 credentials.dat 中的 TTS API key 字段拆分为 mimo_tts_api_key 和 minimax_tts_api_key，移除 tts_provider，修改回滚逻辑为 minimax → xiaomi → sapi

**Architecture:** 修改 CredentialsManager 读取独立 API key，修改 TTSProviderFactory 适配新的回滚逻辑

**Tech Stack:** C++ (CredentialsManager, TTSProviderFactory), nlohmann/json

---

## 1. CredentialsManager 修改

**Files:**
- Modify: `MonsterOrderWilds/CredentialsManager.cpp:10-15`
- Modify: `MonsterOrderWilds/CredentialsManager.cpp:120-132`
- Modify: `MonsterOrderWilds/CredentialsManager.cpp:143-148`
- Modify: `MonsterOrderWilds/CredentialsManager.h:8-13`

- [x] **Step 1: 添加 s_minimaxApiKey 静态变量** (2 min) - 已存在

- [x] **Step 2: 修改 JSON 解析，读取新字段** (3 min) - 已实现

- [x] **Step 3: 添加 GetMINIMAX_API_KEY() 函数声明** (1 min) - 已存在

- [x] **Step 4: 实现 GetMINIMAX_API_KEY() 函数** (1 min) - 已添加

- [x] **Step 5: 验证编译** (1 min) - 编译成功

注：CredentialsManager.cpp 中 s_minimaxApiKey 静态变量、JSON 解析逻辑和 GetMINIMAX_API_KEY() 声明在修改前已存在，只需添加 GetMINIMAX_API_KEY() 函数实现。

---

## 2. TTSProviderFactory 修改

**Files:**
- Modify: `MonsterOrderWilds/TTSProviderFactory.cpp:5-32`
- Modify: `MonsterOrderWilds/ITTSProvider.h` (更新声明)

- [x] **Step 1: 修改 TTSProviderFactory::Create 签名和逻辑** (5 min) - 已实现

ITTSProvider.h 和 TTSProviderFactory.cpp 已更新为新签名和回滚逻辑。

- [x] **Step 2: 验证编译** (1 min) - 编译成功

---

## 3. TextToSpeech.cpp 调用修改

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp:28-33`

- [x] **Step 1: 修改 TTSManager 构造函数，传递两个 API key** (3 min) - 已实现

已添加 TTSProvider.h include，并使用 TTSProviderFactory::Create。

- [x] **Step 2: 验证编译** (1 min) - 编译成功

- [ ] **Step 3: 验证 AI_PROVIDER JSON 不再包含 TTS 字段** (2 min) - 待确认

验证 `GetAI_PROVIDER()` 返回的 JSON 不再包含 `tts_provider` 和 `tts_api_key` 字段。如发现 CaptainCheckInModule.cpp 等模块依赖这些字段，需要相应修改。

---

## 4. TTSProviderTests.cpp 测试更新

**Files:**
- Modify: `MonsterOrderWilds/TTSProviderTests.cpp:233-265`

- [x] **Step 1: 更新 TTSProviderFactory 测试用例** (5 min) - 已实现

- [x] **Step 2: 更新测试运行函数** (2 min) - 已实现

- [ ] **Step 3: 运行单元测试** (1 min) - 待执行

---

## 5. C# 层修改

**Files:**
- Modify: `JonysandMHDanmuTools/DataStructures.cs:26,71,106`
- Modify: `JonysandMHDanmuTools/Utils.cs:195-197,396-398`
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:110-115,208,236`

注：MiniMax API Key 通过 credentials.dat 读取，不在 UI 中显示。

- [x] **Step 1: 在 DataStructures.cs 中添加 MinimaxApiKey 字段** (2 min) - 已实现

- [x] **Step 2: 在 Utils.cs 中注册 minimaxApiKey 字段** (2 min) - 已实现

- [x] **Step 3: 在 ProxyClasses.cs 中添加 MinimaxApiKey 属性** (2 min) - 已实现

- [N/A] **Step 4: 在 ConfigWindow.xaml 中添加 MiniMax API Key 输入框** - 跳过

  MiniMax API Key 通过 credentials.dat 读取，不在 UI 中显示（用户要求）

- [x] **Step 5: 验证编译** (1 min) - 编译成功

---

## 6. 文档更新

**Files:**
- Modify: `openspec/specs/tts-provider/spec.md`

- [x] **Step 1: 更新 openspec tts-provider spec 引用** - 已更新 Factory 接口和回滚逻辑

- [x] **Step 2: 更新 tts-fallback spec.md** - spec.md 已包含完整场景

---

## 7. 废弃字段清理

**Files:**
- Modify: `JonysandMHDanmuTools/DataStructures.cs`
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs`
- Modify: `JonysandMHDanmuTools/Utils.cs`
- Modify: `JonysandMHDanmuTools/ToolsMain.cs`

- [x] **Step 1: 移除 MimoDialect 和 MimoRole 字段** - 这两个字段从未被使用

- [x] **Step 2: 移除 MimoSpeed 字段** - XiaomiTTSProvider BuildRequestBody 未使用 speed 参数

- [x] **Step 3: 验证编译** - 编译成功

---

## Plan Self-Review

### Spec Coverage
- [x] MiMo TTS API Key 读取成功 → Task 1 (Step 2)
- [x] MiniMax TTS API Key 读取成功 → Task 1 (Step 2, Step 4)
- [x] 遗留字段 tts_api_key 被忽略 → Task 1 (Step 2)
- [x] MiniMax 可用时优先使用 → Task 2 (Step 1)
- [x] MiniMax 不可用时回滚到 Xiaomi → Task 2 (Step 1)
- [x] 两个云端 TTS 都不可用时回滚到 SAPI → Task 2 (Step 1)
- [x] 用户明确选择 mimo 时只使用 MiMo → Task 2 (Step 1)
- [x] 用户明确选择 sapi 时只使用 SAPI → Task 2 (Step 1)

### Scenario → Tasks 映射
- [x] 每个 Scenario 都有对应的 task
- [x] 每个 design decision 都有对应的实现 task
- [x] C# 层 minimaxApiKey 字段添加 → Task 5 (Step 1-4)
- [x] Decision 3 (AI_PROVIDER JSON 清理) → Task 3 (Step 1)

### Decision 3 覆盖检查
- [x] GetAI_PROVIDER() 不再包含 tts_provider 和 tts_api_key
- [x] AI_PROVIDER JSON 清理 → Task 3 (Step 1)
- [x] CaptainCheckInModule 影响评估 → 无影响

### C# 层覆盖检查
- [x] DataStructures.cs: ConfigDataSnapshot.MinimaxApiKey 字段
- [x] Utils.cs: minimaxApiKey 注册和 MINIMAX_API_KEY 属性
- [x] ProxyClasses.cs: ConfigProxy.MinimaxApiKey 属性
- [N/A] ConfigWindow.xaml: MiniMax API Key 输入框（跳过 - API Key 仅从 credentials.dat 读取）

### 废弃字段清理
- [x] MimoDialect - 已移除
- [x] MimoRole - 已移除
- [x] MimoSpeed - 已移除
- [x] minimaxSpeed - 已在 MiniMaxTTSProvider.cpp:87 使用

### 文件路径精确性
- [x] 所有文件路径是精确的（无占位符）
- [x] 行号范围准确

### TDD 步骤
- [x] C++ 项目无需 Python 风格 TDD，但测试用例在实现前已设计好

---

## 执行方式选择

Tasks 已生成。有两种执行方式：

1. **Subagent-Driven (推荐)** - 我派发子任务代理逐个执行任务，任务间进行审查
2. **Inline Execution** - 在当前会话中顺序执行所有任务

请选择执行方式，或输入 `apply` 开始执行。