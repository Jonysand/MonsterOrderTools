# TTS Provider 接口

## 概述

定义 TTS (Text-to-Speech) Provider 的抽象接口，支持多 Provider 实现。

## 接口定义

### ITTSProvider

```cpp
struct TTSRequest {
    std::string text;
};

struct TTSResponse {
    std::vector<uint8_t> audioData;  // 音频数据（mp3/wav）
    bool success;                     // 请求是否成功
    std::string errorMsg;             // 错误信息（成功时为空）
};

using TTSCallback = std::function<void(const TTSResponse&)>;

class ITTSProvider {
public:
    virtual ~ITTSProvider() = default;
    virtual std::string GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual void RequestTTS(const TTSRequest& request, TTSCallback callback) = 0;
};
```

### TTSProviderFactory

```cpp
class TTSProviderFactory {
public:
    static std::unique_ptr<ITTSProvider> Create(
        const std::string& mimoApiKey,
        const std::string& minimaxApiKey,
        const std::string& ttsEngine);
};
```

**参数说明**：
- `mimoApiKey`: Xiaomi/MiMo TTS Provider 的 API Key
- `minimaxApiKey`: MiniMax TTS Provider 的 API Key
- `ttsEngine`: TTS 引擎选择，可选值：
  - `"auto"`: 自动模式，优先 minimax，失败后回滚到 xiaomi，最后降级到 sapi
  - `"mimo"`: 强制使用 Xiaomi/MiMo Provider
  - `"sapi"`: 强制使用 Windows SAPI

## Provider 实现

### SapiTTSProvider - Windows SAPI 降级 Provider

当 `AI_PROVIDER` 中的 TTS Provider 不可用或 API Key 为空时，降级到 Windows SAPI。

**API 端点**: 无（本地 Windows API）

**实现方式**: 通过 `TTSManager::SpeakWithSapi()` 调用 Windows SAPI

```cpp
class SapiTTSProvider : public ITTSProvider {
public:
    SapiTTSProvider() = default;
    
    std::string GetProviderName() const override { return "sapi"; }
    
    bool IsAvailable() const override { return true; }
    
    std::string GetLastError() const override { return lastError_; }
    
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override {
        std::wstring wtext = StringProcessor::Utf8ToWstring(request.text);
        
        if (TTSManager::Inst()->SpeakWithSapi(wtext)) {
            TTSResponse response;
            response.success = true;
            response.audioData = {};  // SAPI 直接播放，无需返回音频数据
            callback(response);
        } else {
            TTSResponse response;
            response.success = false;
            response.errorMsg = "SAPI speak failed";
            callback(response);
        }
    }

private:
    std::string lastError_;
};
```

**降级策略**：
1. AUTO 模式下，按 minimax → xiaomi → sapi 的顺序尝试
2. 用户明确选择时尊重选择（mimo 或 sapi）
3. 所有 Provider 都不可用时，降级到 Windows SAPI

### XiaomiTTSProvider

**API 端点**:
- **Endpoint**: `api.xiaomimimo.com`
- **Port**: 443
- **Path**: `/v1/chat/completions`
- **Method**: POST

**请求体**:
```json
{
  "model": "mimo-v2-tts",
  "messages": [
    {
      "role": "assistant",
      "content": "<style>风格</style>待合成内容"
    }
  ],
  "audio": {
    "voice": "mimo_default",
    "format": "wav"
  }
}
```

**可选音色**:
| 音色名 | voice参数 |
|--------|---------|
| MiMo-默认 | mimo_default |
| MiMo-中文女声 | default_zh |
| MiMo-英文女声 | default_en |

**风格控制**:
- 整体风格：将 `<style>风格</style>` 置于目标文本开头
- 细粒度控制：支持音频标签如 `(紧张，深呼吸)`, `(语速加快)`
- **注意**: `mimoStyle` 配置由 `XiaomiTTSProvider` 直接从 `ConfigManager` 读取，不通过 `TTSRequest` 传递

**支持格式**:
- 非流式：`wav`
- 流式：`pcm16`

### MiniMaxTTSProvider

**API 端点**:
- **Endpoint**: `api.minimaxi.com`
- **Port**: 443
- **Path**: `/v1/t2a_v2`
- **Method**: POST

**请求体**:
```json
{
  "model": "speech-2.8-hd",
  "text": "需要合成语音的文本",
  "stream": false,
  "voice_setting": {
    "voice_id": "male-qn-qingse",
    "speed": 1,  // 从 ConfigManager::GetConfig().minimaxSpeed 读取
    "vol": 1,
    "pitch": 0,
    "emotion": "happy"
  },
  "audio_setting": {
    "sample_rate": 32000,
    "bitrate": 128000,
    "format": "mp3",
    "channel": 1
  }
}
```

**响应解析**:
```json
{
  "data": {
    "audio": "hex编码的audio",
    "status": 2
  }
}
```

## Provider 一览

| Provider | 实现类 | 说明 |
|----------|--------|------|
| xiaomi | XiaomiTTSProvider | Xiaomi TTS API |
| minimax | MiniMaxTTSProvider | MiniMax TTS API |
| sapi | SapiTTSProvider | Windows SAPI（降级 fallback） |

## Factory 实现

```cpp
std::unique_ptr<ITTSProvider> TTSProviderFactory::Create(
    const std::string& mimoApiKey,
    const std::string& minimaxApiKey,
    const std::string& ttsEngine) {

    if (ttsEngine == "sapi") {
        return std::make_unique<SapiTTSProvider>();
    }

    if (ttsEngine == "mimo") {
        if (mimoApiKey.empty()) {
            return std::make_unique<SapiTTSProvider>();
        }
        return std::make_unique<XiaomiTTSProvider>(mimoApiKey);
    }

    // AUTO 模式: minimax -> xiaomi -> sapi
    if (!minimaxApiKey.empty()) {
        return std::make_unique<MiniMaxTTSProvider>(minimaxApiKey);
    }

    if (!mimoApiKey.empty()) {
        return std::make_unique<XiaomiTTSProvider>(mimoApiKey);
    }

    return std::make_unique<SapiTTSProvider>();
}
```

**降级策略**：
1. `ttsEngine = "sapi"`: 直接使用 Windows SAPI
2. `ttsEngine = "mimo"`: 强制使用 Xiaomi/MiMo，如果 API Key 为空则降级到 SAPI
3. `ttsEngine = "auto"` (默认): 优先 MiniMax，失败后回滚到 Xiaomi/MiMo，最后降级到 SAPI
4. 所有 Provider 的 API Key 都为空时，使用 Windows SAPI

## 使用示例

```cpp
// 在 TextToSpeech 中使用
class TextToSpeech {
private:
    std::unique_ptr<ITTSProvider> ttsProvider_;

public:
    void Speak(const std::string& text) {
        TTSRequest request;
        request.text = text;
        // 注意：voice、style、speed、pitch、volume 等参数由各 Provider 自己从 ConfigManager 读取

        ttsProvider_->RequestTTS(request, [this](const TTSResponse& response) {
            if (!response.success) {
                LOG_ERROR("TTS failed: %s", response.errorMsg.c_str());
                return;
            }
            PlayAudio(response.audioData);
        });
    }
};
```

## 文件清单

| 文件 | 职责 |
|------|------|
| `MonsterOrderWilds/ITTSProvider.h` | `ITTSProvider` 接口、`TTSRequest`/`TTSResponse` 和 `TTSProviderFactory` |
| `MonsterOrderWilds/SapiTTSProvider.cpp` | Windows SAPI 降级实现 |
| `MonsterOrderWilds/XiaomiTTSProvider.cpp` | Xiaomi TTS 实现 |
| `MonsterOrderWilds/MiniMaxTTSProvider.cpp` | MiniMax TTS 实现 |

## 实现说明

**TTSRequest 设计原则**：
- `TTSRequest` 只包含通用参数：`text`
- 各 Provider 自己从 `ConfigManager` 读取需要的配置：
  - `XiaomiTTSProvider` 读取 `mimoVoice`、`mimoStyle`
  - `MiniMaxTTSProvider` 读取 `minimaxVoiceId`、`minimaxSpeed`
- 这样避免 `TTSRequest` 膨胀，且配置连接更清晰

**注意**：`MimoTTSClient` 与 `XiaomiTTSProvider` 是独立实现的，因为两者的接口定义不同（TTSRequest/TTSResponse 结构体不同）。如果需要复用 MimoTTSClient 的网络请求逻辑，需要进行接口转换。

## 网络实现机制

所有 TTS Provider 使用 `Network::MakeHttpsRequestAsync` 发起异步 HTTP 请求，通过 `condition_variable` + `mutex` 同步等待完成后，再调用 `TTSCallback` 通知调用方。

这种设计使得：
1. 网络请求不会阻塞主线程
2. 回调在请求完成后被调用，符合 `ITTSProvider` 接口约定
3. 实现简洁，无需协程或复杂状态机

### XiaomiTTSProvider 实现

```cpp
Network::MakeHttpsRequestAsync(
    host, port, path, "POST", headers, body, useSSL,
    [&](bool success, const std::string& response, DWORD error) {
        // 存储结果，通知 condition_variable
    });

// 阻塞等待异步完成
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, [&completed]() { return completed; });

// 等待完成后调用 callback
callback(resp);
```

### MiniMaxTTSProvider 实现

MiniMaxTTSProvider 的网络实现与 XiaomiTTSProvider 相同，都使用 `Network::MakeHttpsRequestAsync` + `condition_variable` 模式。

### MimoTTSClient 实现

MimoTTSClient 使用带重试机制的异步请求：

```cpp
// 使用 shared_ptr 包装的 lambda 实现递归重试
auto requestWithRetryPtr = std::make_shared<std::function<void(...)>>(
    [requestWithRetryPtr, ...](...) {
        if (!success && retryCount < maxRetries) {
            retryCount++;
            Network::MakeHttpsRequestAsync(..., *requestWithRetryPtr);
        } else {
            // 完成
        }
    });
Network::MakeHttpsRequestAsync(..., *requestWithRetryPtr);
```
