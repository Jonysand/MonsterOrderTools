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

namespace {
    std::string GetWinHttpErrorString(DWORD errorCode) {
        LPWSTR buffer = nullptr;
        DWORD len = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            GetModuleHandleW(L"winhttp.dll"),
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&buffer,
            0,
            nullptr);
        if (len == 0 || buffer == nullptr) {
            return "Unknown WinHTTP error (code: " + std::to_string(errorCode) + ")";
        }
        std::wstring wstr(buffer, len);
        LocalFree(buffer);
        return std::string(wstr.begin(), wstr.end());
    }
}

MiniMaxTTSProvider::MiniMaxTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string MiniMaxTTSProvider::GetProviderName() const { return "minimax"; }

std::string MiniMaxTTSProvider::GetLastError() const { return lastError_; }

void MiniMaxTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    Network::MakeHttpsRequestAsync(
        TEXT("api.minimaxi.com"),
        443,
        TEXT("/v1/t2a_v2"),
        TEXT("POST"),
        headers,
        body,
        true,
        [this, callback](bool success, const std::string& resp, DWORD error) {
            if (!success || error != 0) {
                lastError_ = "HTTP request failed: " + GetWinHttpErrorString(error);
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

            auto ttsResp = ParseResponse(resp);
            available_ = ttsResp.success && !ttsResp.audioData.empty();
            if (!available_) {
                lastError_ = ttsResp.errorMsg;
            }
            try {
                callback(ttsResp);
            } catch (...) {
            }
        });
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
