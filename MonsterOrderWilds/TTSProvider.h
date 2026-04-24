#pragma once
#include "framework.h"
#include "ITTSProvider.h"

class SapiTTSProvider : public ITTSProvider {
public:
    SapiTTSProvider();
    std::string GetProviderName() const override;
    bool IsAvailable() const override;
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
private:
    std::string lastError_;
};

class XiaomiTTSProvider : public ITTSProvider, public std::enable_shared_from_this<XiaomiTTSProvider> {
public:
    XiaomiTTSProvider(const std::string& apiKey);
    std::string GetProviderName() const override;
    bool IsAvailable() const override { return !apiKey_.empty(); }
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestBody(const TTSRequest& request) const;
    std::string BuildRequestHeaders(const std::string& apiKey) const;
    TTSResponse ParseResponse(const std::string& responseBody, int httpStatusCode) const;
    std::string ProcessStyleTags(const std::string& input) const;
private:
    std::vector<uint8_t> Base64ToBytes(const std::string& base64) const;
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};

class MiniMaxTTSProvider : public ITTSProvider, public std::enable_shared_from_this<MiniMaxTTSProvider> {
public:
    MiniMaxTTSProvider(const std::string& apiKey);
    std::string GetProviderName() const override;
    bool IsAvailable() const override { return !apiKey_.empty(); }
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestBody(const TTSRequest& request) const;
    std::string BuildRequestHeaders(const std::string& apiKey) const;
    TTSResponse ParseResponse(const std::string& responseBody) const;
    std::vector<uint8_t> HexToBytes(const std::string& hex) const;
private:
    std::vector<uint8_t> Base64ToBytes(const std::string& base64) const;
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};

class ManboTTSProvider : public ITTSProvider, public std::enable_shared_from_this<ManboTTSProvider> {
public:
    ManboTTSProvider();
    std::string GetProviderName() const override;
    bool IsAvailable() const override { return available_; }
    void ResetAvailable() { available_ = true; }
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestUrl(const TTSRequest& request) const;
    TTSResponse ParseApiResponse(const std::string& responseBody) const;
private:
    void DownloadAudio(const std::string& audioUrl, TTSCallback callback);
    std::string lastError_;
    bool available_;
};