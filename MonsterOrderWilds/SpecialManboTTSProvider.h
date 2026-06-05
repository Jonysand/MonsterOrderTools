#pragma once
#include "framework.h"
#include "ITTSProvider.h"
#include <string>
#include <memory>

class SpecialManboTTSProvider : public ITTSProvider, public std::enable_shared_from_this<SpecialManboTTSProvider> {
public:
    SpecialManboTTSProvider(const std::string& apiKey);
    std::string GetProviderName() const override;
    bool IsAvailable() const override { return available_; }
    void ResetAvailable() override { available_ = true; }
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestUrl(const TTSRequest& request) const;
    TTSResponse ParseApiResponse(const std::string& responseBody) const;
private:
    void DownloadAudio(const std::string& audioUrl, TTSCallback callback);
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};
