#include "framework.h"
#include "ITTSProvider.h"
#include "TTSProvider.h"
#include "MimoTTSClient.h"
#include "Network.h"
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <condition_variable>
#include <mutex>

#pragma comment(lib, "winhttp.lib")

XiaomiTTSProvider::XiaomiTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string XiaomiTTSProvider::GetProviderName() const { return "xiaomi"; }

bool XiaomiTTSProvider::IsAvailable() const { return available_; }

std::string XiaomiTTSProvider::GetLastError() const { return lastError_; }

void XiaomiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    std::string responseBody;
    std::mutex mtx;
    std::condition_variable cv;
    bool completed = false;
    DWORD httpError = 0;

    Network::MakeHttpsRequestAsync(
        TEXT("api.xiaomimimo.com"),
        443,
        TEXT("/v1/chat/completions"),
        TEXT("POST"),
        headers,
        body,
        true,
        [&](bool success, const std::string& resp, DWORD error) {
            std::lock_guard<std::mutex> lock(mtx);
            responseBody = resp;
            httpError = error;
            completed = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&completed]() { return completed; });

    if (httpError != 0) {
        lastError_ = "HTTP request failed";
        available_ = false;
        TTSResponse resp;
        resp.success = false;
        resp.errorMsg = lastError_;
        callback(resp);
        return;
    }

    auto resp = ParseResponse(responseBody, 200);
    available_ = resp.success;
    if (!resp.success) {
        lastError_ = resp.errorMsg;
    }
    callback({ resp.audioData, resp.success, resp.errorMsg });
}

std::string XiaomiTTSProvider::BuildRequestBody(const TTSRequest& request) const {
    std::string styleTag = request.style.empty() ? "" : "<style>" + request.style + "</style>";
    std::string fullText = styleTag + request.text;

    nlohmann::json j;
    j["model"] = "mimo-v2-tts";
    j["messages"] = {
        {{"role", "assistant"}, {"content", fullText}}
    };
    j["audio"] = {
        {"voice", request.voice.empty() ? "mimo_default" : request.voice},
        {"format", "wav"}
    };
    return j.dump();
}

std::string XiaomiTTSProvider::BuildRequestHeaders(const std::string& apiKey) const {
    return "Content-Type: application/json\r\nAuthorization: Bearer " + apiKey + "\r\n";
}

TTSResponse XiaomiTTSProvider::ParseResponse(const std::string& responseBody, int httpStatusCode) const {
    TTSResponse result;
    result.success = false;

    if (httpStatusCode != 200) {
        result.errorMsg = "HTTP error: " + std::to_string(httpStatusCode);
        return result;
    }

    try {
        auto j = nlohmann::json::parse(responseBody);
        if (j.contains("error")) {
            result.errorMsg = j["error"]["message"].get<std::string>();
            return result;
        }

        std::string audioHex = j["data"]["audio"].get<std::string>();
        result.audioData = HexToBytes(audioHex);
        result.success = true;
    }
    catch (const std::exception& e) {
        result.errorMsg = std::string("Parse error: ") + e.what();
    }
    return result;
}

std::vector<uint8_t> XiaomiTTSProvider::HexToBytes(const std::string& hex) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}
