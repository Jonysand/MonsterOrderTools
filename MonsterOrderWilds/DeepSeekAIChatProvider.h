#pragma once
#include "framework.h"
#include "AIChatProvider.h"
#include <string>

class DeepSeekAIChatProvider : public IAIChatProvider {
public:
    DeepSeekAIChatProvider(const std::string& apiKey);

    std::string GetProviderName() const override;
    bool IsAvailable() const override { return !apiKey_.empty(); }
    std::string GetLastError() const override;
    bool CallAPI(const std::string& prompt, std::string& outResponse) override;
    void CallAPIAsync(const std::string& prompt, std::function<void(bool, const std::string&)> callback);

    // 构造 Chat Completions 请求体（JSON）。
    // 按 DeepSeek 官方文档开启思考模式并设置 low 思考强度：
    //   "thinking": {"type": "enabled"} + "reasoning_effort": "low"
    // 公开为静态方法以便单元测试直接校验请求内容。
    static std::string BuildRequestBody(const std::string& prompt);

private:
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};