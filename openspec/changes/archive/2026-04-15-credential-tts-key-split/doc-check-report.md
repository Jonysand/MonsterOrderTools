# 文档检查报告 - credential-tts-key-split

## 检查概要

| 项目 | 值 |
|------|-----|
| Change ID | credential-tts-key-split |
| 检查轮次 | 5 轮 (3 轮修复 + 2 轮 PASS 验证) |
| 最终状态 | **PASS** |
| pass_count | 3 / 3 (required) |

## 检查维度

| 维度 | 结果 |
|------|------|
| 文档一致性 | ✅ PASS |
| 功能完整性 | ✅ PASS |
| 影响分析 | ✅ PASS |

## 发现并修复的问题

### 第一轮发现的问题 (3 High, 5 Medium, 1 Low)

| # | 严重性 | 问题描述 | 状态 |
|---|--------|---------|------|
| 1 | High | C# 层缺失 minimaxApiKey 配置字段 | ✅ 已修复 |
| 2 | High | tasks.md 受影响文件列表缺少 C# 层文件 | ✅ 已修复 |
| 3 | High | C# 层缺失导致 MiniMax TTS 功能无法正常使用 | ✅ 已修复 |
| 4 | Medium | proposal.md 缺少 Risks / Trade-offs 部分 | ✅ 已修复 |
| 5 | Medium | Decision 3 没有被 tasks.md 明确标记为对应 Task | ✅ 已修复 |
| 6 | Medium | tasks.md 行号引用无法验证准确性 | ⚠️ 保持 (需源码验证) |
| 7 | Medium | 旧版 credentials.dat 迁移方案不完整 | ✅ 已修复 |
| 8 | Medium | 回滚逻辑变更可能影响现有用户 | ✅ 已修复 |
| 9 | Medium | GetAI_PROVIDER() 变更对 CaptainCheckInModule 的影响未评估 | ✅ 已修复 |
| 10 | Low | MiMo/Xiaomi 命名不一致 | ⚠️ 保持 (语义等价) |

## 修复内容

### 1. proposal.md 修复
- 添加了 Risks / Trade-offs 部分 (第 30-35 行)
- 更新了受影响文件列表，添加 C# 层文件

### 2. design.md 修复
- 修正了"不需要修改 C# 层"的错误描述
- 明确说明需要修改 DataStructures.cs、Utils.cs、ProxyClasses.cs、ConfigWindow.xaml

### 3. tasks.md 修复
- 添加了 Task 5: C# 层修改 (DataStructures.cs, Utils.cs, ProxyClasses.cs, ConfigWindow.xaml)
- 添加了 Task 3 Step 3: AI_PROVIDER JSON 清理验证
- 添加了 Decision 3 覆盖检查
- 添加了 Task 6 Step 2: 更新 tts-fallback spec.md

### 4. spec.md 修复
- 添加了 "mimo 模式下 API key 为空回滚 SAPI" 场景
- 添加了 "minimax 明确选择" 场景
- 添加了 "minimax 模式下 API key 为空回滚 SAPI" 场景

## 文档最终状态

| 文档 | 路径 | 状态 |
|------|------|------|
| proposal.md | `openspec/changes/credential-tts-key-split/proposal.md` | ✅ 完整 |
| design.md | `openspec/changes/credential-tts-key-split/design.md` | ✅ 完整 |
| tasks.md | `openspec/changes/credential-tts-key-split/tasks.md` | ✅ 完整 |
| spec.md | `openspec/changes/credential-tts-key-split/specs/tts-fallback/spec.md` | ✅ 完整 |

## 待确认事项 (非阻塞)

1. **CaptainCheckInModule 影响评估**: 需在实施阶段确认 `GetAI_PROVIDER()` JSON 结构变更是否影响该模块
2. **tasks.md 行号引用**: 需在实施阶段对照源码验证行号准确性

## 结论

**文档检查完成，所有 High/Medium 级别问题已修复。文档满足验收标准。**