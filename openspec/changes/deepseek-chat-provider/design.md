# DeepSeek Chat AI Provider - Design

## Context

现有 `AIChatProviderFactory` 支持 MiniMax 作为唯一的 Chat AI Provider。根据 `ai-chat-provider-spec.md` 的接口规范，扩展支持 DeepSeek。

## Goals

1. 复用 `IAIChatProvider` 接口，保持架构一致性
2. 复用 `MiniMaxAIChatProvider` 的 WinHTTP 同步请求实现
3. 保持 Factory 模式的扩展性

## Non-Goals

1. 不修改 C# 配置层
2. 不支持 DeepSeek Reasoning (思考模式)
3. 不支持流式输出

## Decisions

### DeepSeek API 规格

| 项目 | 值 |
|------|---|
| Base URL | `https://api.deepseek.com` |
| Endpoint | `/chat/completions` |
| Model | `deepseek-chat` |
| Provider ID | `deepseek` |

### 请求格式

```json
{
  "model": "deepseek-chat",
  "messages": [
    {"role": "user", "content": "{prompt}"}
  ]
}
```

### 响应解析

```json
{
  "choices": [
    {
      "finish_reason": "stop",
      "message": {
        "content": "AI生成的文本回复"
      }
    }
  ]
}
```

## File Structure

```
MonsterOrderWilds/
├── AIChatProvider.h                    # 现有接口
├── AIChatProviderFactory.cpp           # 现有工厂（需修改）
├── MiniMaxAIChatProvider.cpp          # 现有实现（参考）
├── DeepSeekAIChatProvider.h           # 新增
├── DeepSeekAIChatProvider.cpp         # 新增
└── DeepSeekAIChatProviderTests.cpp    # 新增
```

## Dependencies

- WinHTTP (Windows API)
- nlohmann/json
- 现有 `IAIChatProvider` 接口
- 现有 `AIChatProviderFactory` 工厂

