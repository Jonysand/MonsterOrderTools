# TTS Provider 接口

## 概述

定义 TTS (Text-to-Speech) Provider 的抽象接口，支持多 Provider 实现。

## 接口定义

### ITTSProvider

```cpp
struct TTSRequest {
    std::string text;
    std::string voice;     // 音色 ID（如 "mimo_default"）
    std::string style;    // 风格标签（如 "开心"），为空则使用默认风格
    float speed;          // 语速（0.5-2.0，默认 1.0）
    float pitch;          // 音调（-500-500，默认 0）
    float volume;         // 音量（0.0-1.0，默认 1.0）
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
        request.voice = "male-qn-qingse";
        request.style = "";  // 空表示使用默认风格
        request.speed = 1.0f;
        request.pitch = 0.0f;
        request.volume = 1.0f;
        
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

**注意**：`MimoTTSClient` 与 `XiaomiTTSProvider` 是独立实现的，因为两者的接口定义不同（TTSRequest/TTSResponse 结构体不同）。如果需要复用 MimoTTSClient 的网络请求逻辑，需要进行接口转换。
