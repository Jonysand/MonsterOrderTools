# Credential TTS Key Split - Code Check Report

## 检查概要

| 项目 | 值 |
|------|-----|
| Change ID | credential-tts-key-split |
| 检查阶段 | code_verification |
| 当前轮次 | 1 |
| Pass Count | 1 / 3 |
| 状态 | ⚠️ 需要修复 |

## Goals 验证

| Goal | 状态 | 证据 |
|------|------|------|
| 1. 拆分 TTS API Key 字段 | ✅ 已实现 | `CredentialsManager.cpp:126-127` 读取 `mimo_tts_api_key` 和 `minimax_tts_api_key` |
| 2. 简化 credentials.dat 字段 | ✅ 已实现 | `CredentialsManager.cpp:129-132` 移除 `tts_provider` 和 `tts_api_key` |
| 3. AUTO 模式回滚顺序 | ✅ 已实现 | `TTSProviderFactory.cpp:24-36` 实现 `minimax → xiaomi → sapi` |

## 功能验证

### Decision 1: TTS API Key 拆分
- ✅ `mimo_tts_api_key` 被 CredentialsManager 读取
- ✅ `minimax_tts_api_key` 被 CredentialsManager 读取
- ✅ `GetMINIMAX_API_KEY()` 函数已添加

### Decision 2: TTSProviderFactory 回滚逻辑
- ✅ `Create(mimoApiKey, minimaxApiKey, ttsEngine)` 新签名
- ✅ `ttsEngine = "auto"`: minimax → xiaomi → sapi
- ✅ `ttsEngine = "mimo"`: 仅使用 Xiaomi（忽略 minimax 是否可用）
- ✅ `ttsEngine = "sapi"`: 仅使用 SAPI

### Decision 3: AI_PROVIDER JSON 清理
- ✅ `GetAI_PROVIDER()` 只返回 `chat_provider` 和 `chat_api_key`
- ✅ 不再包含 `tts_provider` 和 `tts_api_key` 字段

### C# 层
- ✅ `DataStructures.cs`: `MinimaxApiKey` 字段已添加
- ✅ `Utils.cs`: `MINIMAX_API_KEY` 属性和 `minimaxApiKey` 注册
- ✅ `ProxyClasses.cs`: `MinimaxApiKey` 属性已添加

## 发现的问题

### 1. 文档过时 (Minor)

| 属性 | 值 |
|------|-----|
| 严重度 | Minor |
| 类别 | documentation_outdated |
| 位置 | `openspec/specs/tts-provider/spec.md:41-46, 185-209` |
| 描述 | spec.md 仍显示旧的 `TTSProviderFactory::Create()` 签名和实现 |
| 影响 | 文档与实现不一致，读者会得到错误的实现信息 |
| 建议修复 | 更新 tts-provider spec.md 以反映新的接口签名和回滚逻辑 |

### 2. Task 6 未完成

| 属性 | 值 |
|------|-----|
| 位置 | `tasks.md:107` |
| 描述 | `更新 openspec tts-provider spec 引用` 标记为"待执行" |
| 影响 | spec 文档与代码实现不一致 |

## 历史遗留问题（不影响本次 PR）

### MimoDialect/MimoRole/MimoAudioFormat 字段

| 属性 | 值 |
|------|-----|
| 严重度 | Design Issue |
| 类别 | missing_config_field |
| 位置 | `Utils.cs MainConfig class` |
| 描述 | 这些字段在 `MainConfig` 中没有属性定义，ConfigFieldRegistry 也没有注册 |
| 分析 | 这些字段从未被持久化机制支持，是历史遗留的设计问题 |
| 影响 | 不影响 credential-tts-key-split PR |
| 建议 | 如需支持这些配置，需要完整实现：MainConfig 属性 + ConfigFieldRegistry 注册 + ApplyTo 写回 |

## Subagent 检查结果汇总

### Agent 1: 功能完整性 ✅
- Goals 1-3 全部实现
- Tasks 1-5 完成
- Task 6 (文档更新) 待完成

### Agent 2: 安全性
- **历史遗留问题**（不在本次 PR 范围内）：
  - 消息队列无锁保护 (TextToSpeech.cpp:75-96)
  - 硬编码密钥种子 (CredentialsManager.h:5-6)
  - API 密钥明文存储 (CredentialsManager.cpp:10-15)
- **credential-tts-key-split 引入的代码**：无新增安全问题

### Agent 3: 代码质量
- TTSProviderFactory 回滚逻辑正确
- C# 层字段映射正确
- **历史遗留问题**：MimoDialect 等字段缺失（见上文）

## 结论

**credential-tts-key-split 代码实现正确，功能完整。**

发现 1 个 Minor 问题（spec.md 文档未更新），建议修复后再次检查。

## 下一步行动

1. ✅ 更新 `openspec/specs/tts-provider/spec.md`
2. 🔄 执行第二轮代码检查
3. ✅ 确认所有 issues 修复后标记为 PASS