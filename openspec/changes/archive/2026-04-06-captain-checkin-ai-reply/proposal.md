# Captain CheckIn AI Reply

## Goal

为弹幕姬新增"学习舰长发言习惯，对其打卡做出AI回复"功能。当舰长发送含打卡触发词的弹幕时，基于其历史发言习惯生成个性化AI回复。

## Why

- 提升舰长互动体验，增加直播间活跃度
- 复用现有 MiniMax API 能力，无需额外服务
- 观众默认都是怪物猎人玩家，AI回复可更具游戏相关性
- Chat 和 TTS 分别支持多 Provider 扩展

## What

### 新增功能

1. **舰长发言学习** - 记录舰长弹幕到用户画像，提取关键词
2. **打卡检测** - 检测含触发词（默认"打卡","签到"）的舰长弹幕
3. **AI回复生成** - 调用 MiniMax API 生成个性化回复，回退到固定模板
4. **连续打卡天数计算** - 模块自己计算，无需宿主传入

### 新增配置

| 字段 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| `ENABLE_CAPTAIN_CHECKIN_AI` | bool | AI学习功能总开关，关闭时不进行学习、统计、AI回复等任何操作 | true |
| `CHECKIN_TRIGGER_WORDS` | string | 打卡触发词，逗号分隔 | "打卡,签到" |

**说明**：`AI_PROVIDER` 是 credential 字段，由 C++ `CredentialsManager` 独立管理，不经过 C# 配置层。

**Credential JSON 格式**（C++ 独立处理）:
```json
{
  "tts_provider": "xiaomi",
  "tts_api_key": "xxxx",
  "chat_provider": "minimax",
  "chat_api_key": "xxx"
}
```

**TTS Provider 选项**: `xiaomi`, `minimax`

**Chat Provider 选项**: `minimax`

### AI Provider 选择理由

**Chat Provider - MiniMax**：
- 复用现有 MiniMax API 能力，无需额外申请
- 支持文本对话生成
- 社区风气适合游戏玩家互动

**TTS Provider - Xiaomi / MiniMax**：
- Xiaomi：小摩拜合作厂商，API 成熟稳定
- MiniMax：与 Chat Provider 同一厂商，生态统一
- 用户可根据已有 API Key 选择

### 约束条件

- 仅处理 guard_level ≥ 3 的舰长弹幕
- AI回复始终启用，API Key为空时回退固定模板
- 不清理学习记录（简化设计）
- API Key 使用凭证加密存储机制
- Chat 和 TTS 分别选择 Provider，分别读取 API Key

## Scope

### In Scope

- C++ 核心模块 `CaptainCheckInModule`
- 配置字段 C++/C# 两层桥接
- C# UI 配置界面
- DanmuProcessor 集成
- 单元测试

### Out of Scope

- 本地AI模型支持（仅在线API）
- 用户画像自动清理机制
- 其他用户的AI回复（仅舰长）
