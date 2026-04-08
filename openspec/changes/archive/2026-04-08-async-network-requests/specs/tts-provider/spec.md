# Delta Spec: tts-provider 异步化

## 概述

本文档描述 `tts-provider` capability 的网络请求异步化变更。

## 变更类型

**网络请求方式变更**：从同步 WinHTTP API 改为纯异步 callback 模式，对外接口语义不变。

## 详细变更

### Requirement: 网络请求异步化

**变更前**：
```markdown
#### Scenario: TTS API 请求
- **WHEN** 系统需要合成语音
- **THEN** 系统使用同步 WinHTTP API 发送请求，阻塞调用线程直到响应返回
```

**变更后**：
```markdown
#### Scenario: TTS API 请求
- **WHEN** 系统需要合成语音
- **THEN** 系统使用 `Network::MakeHttpsRequestAsync` 发送异步请求，通过 callback 接收响应
```

### 变更原因

原实现使用同步 WinHTTP API（`WinHttpSendRequest` + `WinHttpReceiveResponse` 阻塞调用），会阻塞调用线程。改为异步模式后，网络 I/O 不阻塞线程，更符合纯异步架构要求。

### 实现方式

所有 TTS Provider（MiniMaxTTSProvider、XiaomiTTSProvider）内部使用 `condition_variable` 等待异步完成，保持 `RequestTTS` 方法的同步语义。

### 更新的文件清单

| 文件 | 变更 |
|------|------|
| `MonsterOrderWilds/MiniMaxTTSProvider.cpp` | 改用 `MakeHttpsRequestAsync`，删除 `MakeSyncHttpsRequest` |
| `MonsterOrderWilds/XiaomiTTSProvider.cpp` | 改用 `MakeHttpsRequestAsync`，删除 `MakeSyncHttpsRequest` |
| `MonsterOrderWilds/TTSProvider.h` | 方法声明更新（`HexToBytes` → `Base64ToBytes`） |
| `specs/tts-provider/spec.md` | 新增"网络实现机制"章节，描述异步实现模式 |
