# Delta Spec: ai-chat-provider 异步化

## 概述

本文档描述 `ai-chat-provider` capability 的变更。

## 变更类型

**网络请求方式变更**：内部实现从同步 WinHTTP 改为异步调用 `Network::MakeHttpsRequestAsync`，对外接口 `CallAPI` 保持同步语义不变。

## 详细变更

### 接口变更

| 项目 | 变更前 | 变更后 |
|------|--------|--------|
| `IAIChatProvider::CallAPI` | 同步接口 | 保持同步接口（内部实现异步化） |
| 网络请求实现 | 私有 `MakeSyncHttpsRequest` 同步方法 | 私有 `MakeHttpsRequestAsync` 异步方法 |

### 实现要求变更

**变更前**（spec.md 第 211-225 行）：
```cpp
bool MakeSyncHttpsRequest(...);  // 必须使用 WinHTTP 同步 API
```

**变更后**：
```cpp
void MakeHttpsRequestAsync(..., std::function<void(bool, const std::string&)> callback);
```

内部通过 `condition_variable` 等待异步完成，对外保持 `CallAPI` 同步语义。

### 移除内容

- 删除 spec.md 中"同步请求实现要求"章节（第 211-225 行）
- 删除 `MakeSyncHttpsRequest` 方法声明和注释

### 更新的文件清单

| 文件 | 变更 |
|------|------|
| `specs/ai-chat-provider/spec.md` | 移除同步要求章节，更新实现说明 |
