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

class XiaomiTTSProvider : public ITTSProvider {
public:
    XiaomiTTSProvider(const std::string& apiKey);
    std::string GetProviderName() const override;
    bool IsAvailable() const override;
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestBody(const TTSRequest& request) const;
    std::string BuildRequestHeaders(const std::string& apiKey) const;
    TTSResponse ParseResponse(const std::string& responseBody, int httpStatusCode) const;
private:
    std::vector<uint8_t> HexToBytes(const std::string& hex) const;
    bool MakeSyncHttpsRequest(const std::string& host, int port, const std::string& path,
        const std::string& headers, const std::string& body, std::string& outResponse);
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};

class MiniMaxTTSProvider : public ITTSProvider {
public:
    MiniMaxTTSProvider(const std::string& apiKey);
    std::string GetProviderName() const override;
    bool IsAvailable() const override;
    std::string GetLastError() const override;
    void RequestTTS(const TTSRequest& request, TTSCallback callback) override;
    std::string BuildRequestBody(const TTSRequest& request) const;
    std::string BuildRequestHeaders(const std::string& apiKey) const;
    TTSResponse ParseResponse(const std::string& responseBody) const;
private:
    std::vector<uint8_t> HexToBytes(const std::string& hex) const;
    bool MakeSyncHttpsRequest(const std::string& host, int port, const std::string& path,
        const std::string& headers, const std::string& body, std::string& outResponse);
    std::string apiKey_;
    std::string lastError_;
    bool available_;
};