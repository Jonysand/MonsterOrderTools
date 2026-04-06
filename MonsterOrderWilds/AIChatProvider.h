#pragma once
#include "framework.h"
#include <string>
#include <memory>
#include <functional>

class IAIChatProvider {
public:
    virtual ~IAIChatProvider() = default;
    virtual std::string GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetLastError() const = 0;
    virtual bool CallAPI(const std::string& prompt, std::string& outResponse) = 0;
};

class AIChatProviderFactory {
public:
    static std::unique_ptr<IAIChatProvider> Create(const std::string& credentialJson);
};