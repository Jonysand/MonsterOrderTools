#pragma once
#include "framework.h"
#include "AIChatProvider.h"
#include <string>

class MiniMaxAIChatProvider : public IAIChatProvider {
public:
    MiniMaxAIChatProvider(const std::string& apiKey);

    std::string GetProviderName() const override;
    bool IsAvailable() const override;
    std::string GetLastError() const override;
    bool CallAPI(const std::string& prompt, std::string& outResponse) override;

private:
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};