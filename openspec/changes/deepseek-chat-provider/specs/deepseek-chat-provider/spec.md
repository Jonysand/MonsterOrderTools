# DeepSeek Chat AI Provider 接口规格

## Overview

定义 DeepSeek Chat AI Provider，实现 `IAIChatProvider` 接口。

## ADDED Requirements

### Requirement: DeepSeekAIChatProvider 继承 IAIChatProvider 接口

#### Scenario: Provider 名称返回 "deepseek"
- **WHEN** 调用 `DeepSeekAIChatProvider::GetProviderName()`
- **THEN** 返回字符串 `"deepseek"`

#### Scenario: 可用状态由 API 调用结果决定
- **WHEN** 构造 `DeepSeekAIChatProvider` 实例
- **THEN** `IsAvailable()` 返回 `false`（初始状态）
- **WHEN** `CallAPI()` 调用成功
- **THEN** `IsAvailable()` 返回 `true`

### Requirement: DeepSeek API 调用

#### Scenario: 构建正确的请求体
- **WHEN** 调用 `CallAPI("Hello", response)`
- **THEN** 发送 HTTP POST 请求到 `https://api.deepseek.com/chat/completions`
- **AND** 请求头包含 `Content-Type: application/json`
- **AND** 请求头包含 `Authorization: Bearer {apiKey}`
- **AND** 请求体为 `{"model":"deepseek-chat","messages":[{"role":"user","content":"Hello"}]}`

#### Scenario: 成功解析响应
- **WHEN** API 返回 `{"choices":[{"message":{"content":"Hi"}}]}`
- **THEN** `outResponse` 被设置为 `"Hi"`
- **AND** `CallAPI()` 返回 `true`

#### Scenario: 解析失败时设置错误信息
- **WHEN** API 返回无效 JSON
- **THEN** `GetLastError()` 返回包含 "JSON parse error" 的错误信息
- **AND** `CallAPI()` 返回 `false`

### Requirement: AIChatProviderFactory 支持 DeepSeek

#### Scenario: Factory 根据 chat_provider 创建 DeepSeek Provider
- **WHEN** 调用 `Create(R"({"chat_provider":"deepseek","chat_api_key":"test_key"})")`
- **THEN** 返回非空 `DeepSeekAIChatProvider` 实例

#### Scenario: 空 API Key 返回 nullptr
- **WHEN** 调用 `Create(R"({"chat_provider":"deepseek","chat_api_key":""})")`
- **THEN** 返回 `nullptr`