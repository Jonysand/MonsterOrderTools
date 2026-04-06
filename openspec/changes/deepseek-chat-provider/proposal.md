# DeepSeek Chat AI Provider

## Why

- 为舰长打卡 AI 回复功能提供更多 AI Provider 选择
- DeepSeek API 成本较低，模型能力强
- 支持与 MiniMax 共存的 Provider 扩展架构

## What Changes

新增 DeepSeek 作为 Chat AI Provider 选项，复用现有 `IAIChatProvider` 接口和 `AIChatProviderFactory` 工厂模式。

## Capabilities

### New Capabilities
- `deepseek-chat-provider`: DeepSeek Chat Completion API 支持

## Impact

- 新增 3 个文件：`DeepSeekAIChatProvider.h/cpp`, `DeepSeekAIChatProviderTests.cpp`
- 修改 1 个文件：`AIChatProviderFactory.cpp`（添加 Provider 分支）
- 修改 3 个 vcxproj/filters 文件

## Non-Goals

- 不修改 C# 配置界面（AI_PROVIDER 作为 credential 字段由 C++ 独立管理）
- 不支持 DeepSeek Reasoning (思考模式)
- 不支持流式输出