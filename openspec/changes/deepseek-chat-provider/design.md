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

## Additional Design Decisions

### MiniMaxAIChatProvider.cpp 变量名规范化

在实现 DeepSeek 时，发现 `MiniMaxAIChatProvider::CallAPI()` 中使用变量名 `j` 不够清晰。

| 项目 | 修改前 | 修改后 |
|------|--------|--------|
| 变量名 | `j` | `responseJson` |

**理由**：DeepSeek 和 MiniMax 共用相同的 JSON 解析模式，统一使用更清晰的变量名便于维护。

### MonsterOrderWildsGUI.csproj 配置完善

为 GUI 项目添加了完整的 `PropertyGroup` 配置（Debug/Release/UnitTest），确保在不同编译配置下行为一致。

**理由**：原有 GUI 项目只有 `AnyCPU` 配置，添加 `x64` 平台支持以匹配 C++ 项目的编译配置体系。

### BliveManagerTests.vcxproj 修改

在 `BliveManagerTests.vcxproj` 中添加了 DeepSeek 相关的测试文件引用。

**理由**：DeepSeekAIChatProviderTests 需要被包含在 BliveManagerTests 项目中以支持完整的测试运行。

### AIChatProviderTests 编译条件调整

| 配置 | 修改前 | 修改后 |
|------|--------|--------|
| ExcludedFromBuild | `'$(Configuration)'=='Release'` | `'$(Configuration)'!='UnitTest'` |

**理由**：统一单元测试的编译条件，确保 AIChatProviderTests 和 DeepSeekAIChatProviderTests 在 Debug 配置下不被编译（保持一致），仅在 UnitTest 配置下编译运行。