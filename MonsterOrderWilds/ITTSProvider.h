#pragma once
#include "framework.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct TTSRequest {
    std::string text;
};

struct TTSResponse {
    std::vector<uint8_t> audioData;
    bool success;
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
    virtual void ResetAvailable() {}
};

class TTSProviderFactory {
public:
    static std::shared_ptr<ITTSProvider> Create(
        const std::string& mimoApiKey,
        const std::string& minimaxApiKey,
        const std::string& ttsEngine);
};
