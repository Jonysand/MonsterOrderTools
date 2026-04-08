#include "framework.h"
#include "TTSProvider.h"
#include "Network.h"
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <condition_variable>
#include <mutex>

#pragma comment(lib, "winhttp.lib")

MiniMaxTTSProvider::MiniMaxTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string MiniMaxTTSProvider::GetProviderName() const { return "minimax"; }

bool MiniMaxTTSProvider::IsAvailable() const { return available_; }

std::string MiniMaxTTSProvider::GetLastError() const { return lastError_; }

void MiniMaxTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    std::string responseBody;
    std::mutex mtx;
    std::condition_variable cv;
    bool completed = false;
    DWORD httpError = 0;

    Network::MakeHttpsRequestAsync(
        TEXT("api.minimaxi.com"),
        443,
        TEXT("/v1/t2a_v2"),
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

    auto resp = ParseResponse(responseBody);
    available_ = resp.success && !resp.audioData.empty();
    if (!available_) {
        lastError_ = resp.errorMsg;
    }
    try {
        callback(resp);
    } catch (...) {
    }
}

std::string MiniMaxTTSProvider::BuildRequestBody(const TTSRequest& request) const {
    nlohmann::json j;
    j["model"] = "speech-2.8-hd";
    j["text"] = request.text;
    j["stream"] = false;
    j["voice_setting"] = {
        {"voice_id", request.voice.empty() ? "male-qn-qingse" : request.voice},
        {"speed", request.speed > 0 ? request.speed : 1.0f},
        {"vol", 1},
        {"pitch", 0},
        {"emotion", "happy"}
    };
    j["audio_setting"] = {
        {"sample_rate", 32000},
        {"bitrate", 128000},
        {"format", "mp3"},
        {"channel", 1}
    };
    return j.dump();
}

std::string MiniMaxTTSProvider::BuildRequestHeaders(const std::string& apiKey) const {
    return "Content-Type: application/json\r\nAuthorization: Bearer " + apiKey + "\r\n";
}

TTSResponse MiniMaxTTSProvider::ParseResponse(const std::string& responseBody) const {
    TTSResponse result;
    result.success = false;
    result.audioData = {};

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

        if (j.contains("data") && j["data"].contains("audio") && j["data"]["audio"].is_string()) {
            std::string audioBase64 = j["data"]["audio"].get<std::string>();
            result.audioData = Base64ToBytes(audioBase64);
            result.success = true;
        }
        else {
            result.errorMsg = "Invalid response format";
        }
    }
    catch (const std::exception& e) {
        result.errorMsg = std::string("Parse error: ") + e.what();
    }
    return result;
}

std::vector<uint8_t> MiniMaxTTSProvider::Base64ToBytes(const std::string& base64) const {
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
