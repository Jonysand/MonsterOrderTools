# Delta Spec: mimo-tts-integration 异步化

## 概述

本文档描述 `mimo-tts-integration` capability 的网络请求异步化变更。

## 变更类型

**异步处理方式变更**：从协程模式改为 callback 模式，对外接口语义不变。

## 详细变更

### Requirement: 异步处理

**变更前**：
```markdown
#### Scenario: 异步API调用
- **WHEN** 系统需要合成语音
- **THEN** 系统在后台协程中处理API请求，不阻塞主线程
```

**变更后**：
```markdown
#### Scenario: 异步API调用
- **WHEN** 系统需要合成语音
- **THEN** 系统通过 WinHTTP 异步 callback 处理API请求，不阻塞主线程
```

### 变更原因

原实现使用协程 `Network::MakeHttpsRequest`，需要协程调度器。改为 callback 模式后，不依赖协程调度器，更符合纯异步架构要求。

### 更新的文件清单

| 文件 | 变更 |
|------|------|
| `specs/mimo-tts-integration/spec.md` | 更新异步处理描述，从"协程"改为"callback" |
