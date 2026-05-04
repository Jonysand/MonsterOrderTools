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
        
        std::mutex mtx;
        std::condition_variable cv;
        bool completed = false;
        std::string response;
        
        Network::MakeHttpsRequestAsync(
            utf8_to_wstring("api.minimaxi.com"),
            443,
            utf8_to_wstring("/v1/text/chatcompletion_v2"),
            TEXT("POST"),
            headersStr,
            body,
            true,
            [&](bool success, const std::string& respBody, DWORD error) {
                if (success) {
                    response = respBody;
                }
                std::lock_guard<std::mutex> lock(mtx);
                completed = true;
                cv.notify_one();
            }
        );
        
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&completed]() { return completed; });
        
        if (response.empty()) {
            lastError_ = "HTTP request failed";
            available_ = false;
            return false;
        }
        
        try {
            json j = json::parse(response);
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
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};
```

**注意**: `CallAPI` 内部使用 `Network::MakeHttpsRequestAsync` 进行异步 HTTP 请求，通过 `condition_variable` 等待异步完成。接口语义保持同步，但内部实现为纯异步 callback 模式。

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
| `MonsterOrderWilds/MiniMaxAIChatProvider.cpp` | MiniMax API 实现（使用 `Network::MakeHttpsRequestAsync`） |
