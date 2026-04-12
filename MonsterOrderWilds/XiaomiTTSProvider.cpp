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
            try {
                std::lock_guard<std::mutex> lock(mtx);
                responseBody = resp;
                httpError = error;
                completed = true;
                cv.notify_one();
            } catch (...) {
                // Mutex operations should not throw, but handle defensively
            }
        });

    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&completed]() { return completed; });

    if (httpError != 0) {
        lastError_ = "HTTP request failed";
        available_ = false;
        TTSResponse resp;
        resp.success = false;
        resp.errorMsg = lastError_;
        try {
            callback(resp);
        } catch (...) {
        }
        return;
    }

    auto resp = ParseResponse(responseBody, 200);
    available_ = resp.success;
    if (!resp.success) {
        lastError_ = resp.errorMsg;
    }
    try {
        callback({ resp.audioData, resp.success, resp.errorMsg });
    } catch (...) {
    }
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
            if (j["error"].contains("message") && j["error"]["message"].is_string()) {
                result.errorMsg = j["error"]["message"].get<std::string>();
            } else {
                result.errorMsg = "Unknown error";
            }
            return result;
        }

        if (!j.contains("choices") || !j["choices"].is_array() || j["choices"].empty()) {
            result.errorMsg = "No choices in response";
            return result;
        }

        auto& choice = j["choices"][0];
        if (!choice.contains("message") || !choice["message"].contains("audio")) {
            result.errorMsg = "No audio in response";
            return result;
        }

        auto& audio = choice["message"]["audio"];
        if (!audio.contains("data") || !audio["data"].is_string()) {
            result.errorMsg = "No audio data in response";
            return result;
        }

        std::string base64Audio = audio["data"].get<std::string>();
        result.audioData = Base64ToBytes(base64Audio);
        result.success = true;
    }
    catch (const std::exception& e) {
        result.errorMsg = std::string("Parse error: ") + e.what();
    }
    return result;
}

std::vector<uint8_t> XiaomiTTSProvider::Base64ToBytes(const std::string& base64) const {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::vector<uint8_t> decoded;
    int in_len = base64.size();
    int i = 0, in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    
    while (in_len-- && (base64[in_] != '=') && 
           (isalnum(base64[in_]) || (base64[in_] == '+') || (base64[in_] == '/'))) {
        char_array_4[i++] = base64[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                decoded.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (int j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++)
            decoded.push_back(char_array_3[j]);
    }
    
    return decoded;
}
