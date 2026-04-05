# TTS Provider 接口

## 概述

定义 TTS (Text-to-Speech) Provider 的抽象接口，支持多 Provider 实现。

## 接口定义

### ITTSProvider

```cpp
struct TTSRequest {
    std::string text;
    std::string voiceId;
    float speed;
    float pitch;
    float volume;
};

struct TTSResponse {
    std::vector<uint8_t> audioData;
    std::string errorMsg;
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
    static std::unique_ptr<ITTSProvider> Create(const std::string& credentialJson);
};
```

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
        std::wstring wtext = StringUtil::Utf8ToWide(request.text);
        
        if (TTSManager::Inst()->SpeakWithSapi(wtext.c_str())) {
            TTSResponse response;
            response.audioData = {};  // SAPI 直接播放，无需返回音频数据
            callback(response);
        } else {
            TTSResponse response;
            response.errorMsg = "SAPI speak failed";
            callback(response);
        }
    }

private:
    std::string lastError_;
};
```

**降级策略**：
1. 优先使用 `AI_PROVIDER` 中指定的 TTS Provider（xiaomi 或 minimax）
2. 如果 Provider 不可用或 API Key 为空，降级到 Windows SAPI
3. Windows SAPI 通过 `TTSManager::SpeakWithSapi()` 实现

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
    "speed": 1,
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
    const std::string& credentialJson) {
    
    try {
        json cred = json::parse(credentialJson);
        
        std::string provider = cred.value("tts_provider", "xiaomi");
        std::string apiKey = cred.value("tts_api_key", "");
        
        if (!apiKey.empty()) {
            if (provider == "xiaomi") {
                return std::make_unique<XiaomiTTSProvider>(apiKey);
            } else if (provider == "minimax") {
                return std::make_unique<MiniMaxTTSProvider>(apiKey);
            }
        }
        
        // API Key 为空或 Provider 不支持时，降级到 Windows SAPI
        return std::make_unique<SapiTTSProvider>();
    } catch (...) {
        return nullptr;
    }
}
```

**降级策略**：
1. 优先使用 `AI_PROVIDER` 中指定的 TTS Provider
2. 如果 API Key 为空或 Provider 不支持，降级到 Windows SAPI
3. Windows SAPI 通过 `TTSManager::SpeakWithSapi()` 实现

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
        request.voiceId = "male-qn-qingse";
        request.speed = 1.0f;
        request.pitch = 0.0f;
        request.volume = 1.0f;
        
        ttsProvider_->RequestTTS(request, [this](const TTSResponse& response) {
            if (!response.errorMsg.empty()) {
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

## 优化建议：复用现有 MimoTTSClient

**问题**：`MimoTTSClient` 已经实现了完整的 Xiaomi TTS API 调用逻辑。

**建议**：让 `XiaomiTTSProvider` 内部复用 `MimoTTSClient` 的实现，而不是重新实现一遍。

```cpp
// XiaomiTTSProvider 优化方案
class XiaomiTTSProvider : public ITTSProvider {
private:
    std::unique_ptr<MimoTTSClient> mimoClient_;  // 复用现有客户端
    
public:
    XiaomiTTSProvider(const std::string& apiKey) {
        mimoClient_ = std::make_unique<MimoTTSClient>();
    }
    
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override {
        // 转换 TTSRequest 到 MimoTTSClient::TTSRequest
        MimoTTSClient::TTSRequest req;
        req.text = request.text;
        req.voice = request.voiceId;
        req.speed = request.speed;
        // ... 其他字段映射
        
        mimoClient_->RequestTTS(req, [callback](const MimoTTSClient::TTSResponse& resp) {
            // 转换响应并调用 callback
            TTSResponse result;
            result.success = resp.success;
            result.audioData = resp.audioData;
            result.errorMessage = resp.errorMessage;
            callback(result);
        });
    }
};
```

**注意**：此为可选优化，不影响功能正确性。如果 `MimoTTSClient` 的接口不满足需求，可以保持独立实现。
