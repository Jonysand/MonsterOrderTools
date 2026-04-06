# AI Chat Provider 接口

## 概述

定义文本对话 AI Provider 的抽象接口，支持多 Provider 实现。

## 接口定义

### IAIChatProvider

```cpp
class IAIChatProvider {
public:
    virtual ~IAIChatProvider() = default;
    virtual std::string GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual bool CallAPI(const std::string& prompt, std::string& outResponse) = 0;
};
```

### AIChatProviderFactory

```cpp
class AIChatProviderFactory {
public:
    static std::unique_ptr<IAIChatProvider> Create(const std::string& credentialJson);
};
```

## Provider 实现

### MiniMaxAIChatProvider

**API 端点**:
- **Endpoint**: `api.minimaxi.com`
- **Port**: 443
- **Path**: `/v1/text/chatcompletion_v2`
- **Method**: POST

**请求头**:
```
Content-Type: application/json
Authorization: Bearer {CHAT_AI_API_KEY}
```

**请求体**:
```json
{
  "model": "M2-her",
  "messages": [
    {"role": "user", "content": "{prompt}"}
  ]
}
```

**响应解析**:
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

**实现代码**:

```cpp
class MiniMaxAIChatProvider : public IAIChatProvider {
public:
    MiniMaxAIChatProvider(const std::string& apiKey)
        : apiKey_(apiKey), available_(false) {}
    
    std::string GetProviderName() const override { return "minimax"; }
    
    bool IsAvailable() const override { return available_; }
    
    std::string GetLastError() const override { return lastError_; }
    
    bool CallAPI(const std::string& prompt, std::string& outResponse) override {
        std::string body = R"({
            "model": "M2-her",
            "messages": [{"role": "user", "content": ")" + prompt + R"("}]
        })";
        
        std::string headersStr = 
            "Content-Type: application/json\r\n"
            "Authorization: Bearer " + apiKey_ + "\r\n";
        
        // 使用 WinHTTP 同步请求（不依赖协程）
        bool success = MakeSyncHttpsRequest(
            "api.minimaxi.com",
            443,
            "/v1/text/chatcompletion_v2",
            headersStr,
            body,
            outResponse
        );
        
        if (!success) {
            lastError_ = "HTTP request failed";
            available_ = false;
            return false;
        }
        
        // 解析 JSON 响应
        try {
            json j = json::parse(outResponse);
            outResponse = j["choices"][0]["message"]["content"].get<std::string>();
            available_ = true;
            return true;
        } catch (const std::exception& e) {
            lastError_ = std::string("JSON parse error: ") + e.what();
            available_ = false;
            return false;
        }
    }
    
private:
    // 同步 HTTPS 请求（封装 WinHTTP）
    bool MakeSyncHttpsRequest(
        const std::string& host,
        int port,
        const std::string& path,
        const std::string& headers,
        const std::string& body,
        std::string& outResponse);
    
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};
```

**注意**: `MakeSyncHttpsRequest` 是新增的同步封装函数，内部使用 WinHTTP API 实现同步 HTTP 请求，不能使用 `Network::MakeHttpsRequest`（协程函数）。参考 `MimoTTSClient.cpp` 中的同步实现方式。

### Factory 实现

```cpp
std::unique_ptr<IAIChatProvider> AIChatProviderFactory::Create(
    const std::string& credentialJson) {
    
    try {
        json cred = json::parse(credentialJson);
        
        std::string provider = cred.value("chat_provider", "minimax");
        std::string apiKey = cred.value("chat_api_key", "");
        
        if (apiKey.empty()) {
            return nullptr;
        }
        
        if (provider == "minimax") {
            return std::make_unique<MiniMaxAIChatProvider>(apiKey);
        }
        
        // 预留其他 Provider
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}
```

## 使用示例

```cpp
// 在 CaptainCheckInModule 中使用
bool CaptainCheckInModule::GenerateAIAnswer(
    const CheckinEvent& event, 
    const UserProfile* profile,
    std::string& outAnswer) {
    
    // 获取 AI Provider
    std::string credJson = CredentialsManager::Inst()->GetAI_PROVIDER();
    auto provider = AIChatProviderFactory::Create(credJson);
    
    if (!provider || !provider->IsAvailable()) {
        return false;
    }
    
    // 构建 Prompt
    std::string prompt = BuildPrompt(event, profile);
    
    // 调用 API
    return provider->CallAPI(prompt, outAnswer);
}
```

## 错误处理

| 错误类型 | 处理方式 |
|----------|----------|
| API Key 为空 | 返回 `nullptr`，调用方使用固定模板 |
| HTTP 请求失败 | 设置 `lastError_`，返回 `false` |
| JSON 解析失败 | 设置 `lastError_`，返回 `false` |
| 网络超时 | 设置 `lastError_`，返回 `false` |

## 文件清单

| 文件 | 职责 |
|------|------|
| `MonsterOrderWilds/AIChatProvider.h` | `IAIChatProvider` 接口和 `AIChatProviderFactory` |
| `MonsterOrderWilds/MiniMaxAIChatProvider.cpp` | MiniMax API 实现（含同步 WinHTTP 封装） |

## 同步请求实现要求

`MakeSyncHttpsRequest` 必须使用 WinHTTP 同步 API 实现，不能使用 `Network::MakeHttpsRequest`（协程函数）。

参考实现方式（参考 `MimoTTSClient.cpp`）：
```cpp
bool MiniMaxAIChatProvider::MakeSyncHttpsRequest(
    const std::string& host, int port, const std::string& path,
    const std::string& headers, const std::string& body,
    std::string& outResponse) {
    // 使用 WinHttpOpen, WinHttpConnect, WinHttpOpenRequest, WinHttpSendRequest, WinHttpReceiveResponse
    // 实现同步 HTTP POST 请求
    // 返回响应 body 到 outResponse
}
```
