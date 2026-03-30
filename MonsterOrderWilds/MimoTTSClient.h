#pragma once
#include "framework.h"
#include "Network.h"

// MiMo TTS API client
class MimoTTSClient
{
public:
    struct TTSRequest
    {
        std::string text;
        std::string voice;
        std::string model;
        std::string responseFormat;
        float speed;
        std::string style;
        std::string dialect;
        std::string role;

        TTSRequest()
            : model("mimo-v2-tts")
            , voice("mimo_default")
            , responseFormat("mp3")
            , speed(1.0f)
        {}
    };

    struct TTSResponse
    {
        bool success;
        std::string errorMessage;
        std::vector<uint8_t> audioData;
        int httpStatusCode;
        int retryCount;

        TTSResponse()
            : success(false)
            , httpStatusCode(0)
            , retryCount(0)
        {}
    };

    using TTSCallback = std::function<void(const TTSResponse&)>;

    MimoTTSClient();
    ~MimoTTSClient();

    void RequestTTS(const TTSRequest& request, TTSCallback callback);
    bool IsAvailable() const;
    std::string GetLastError() const;
    void CleanupCoroutines();

private:
    void ExecuteWithRetry(const TTSRequest& request, TTSCallback callback, int maxRetries = 3);

public:
    std::string BuildRequestBody(const TTSRequest& request) const;
    std::string BuildRequestHeaders(const std::string& apiKey) const;
    TTSResponse ParseResponse(const std::string& responseBody, int httpStatusCode) const;

    // API endpoint configuration
    static constexpr const char* API_ENDPOINT = "api.xiaomimimo.com";
    static constexpr INTERNET_PORT API_PORT = 443;
    static constexpr const char* API_PATH = "/v1/chat/completions";

    std::string lastError;
    std::list<Network::NetworkCoroutine> activeCoroutines;
};
