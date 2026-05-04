#pragma once
#include "framework.h"
#include "AIChatProvider.h"
#include <string>

class MiniMaxAIChatProvider : public IAIChatProvider {
public:
    MiniMaxAIChatProvider(const std::string& apiKey);

    std::string GetProviderName() const override;
    bool IsAvailable() const override { return !apiKey_.empty(); }
    std::string GetLastError() const override;
    bool CallAPI(const std::string& prompt, std::string& outResponse) override;
    void CallAPIAsync(const std::string& prompt, std::function<void(bool, const std::string&)> callback);

private:
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};