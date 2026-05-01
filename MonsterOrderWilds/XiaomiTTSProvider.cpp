#include "framework.h"
#include "ITTSProvider.h"
#include "TTSProvider.h"
#include "ConfigManager.h"
#include "Network.h"
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <regex>

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

XiaomiTTSProvider::XiaomiTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(true) {}

std::string XiaomiTTSProvider::GetProviderName() const { return "xiaomi"; }



std::string XiaomiTTSProvider::GetLastError() const { return lastError_; }

void XiaomiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    Network::MakeHttpsRequestAsync(
        TEXT("api.xiaomimimo.com"),
        443,
        TEXT("/v1/chat/completions"),
        TEXT("POST"),
        headers,
        body,
        true,
        [self = shared_from_this(), callback](bool success, const std::string& resp, DWORD error) {
            if (!success || error != 0) {
                self->lastError_ = "HTTP request failed: " + GetWinHttpErrorString(error);
                self->available_ = false;
                TTSResponse ttsResp;
                ttsResp.success = false;
                ttsResp.errorMsg = self->lastError_;
                try {
                    callback(ttsResp);
                } catch (...) {
                }
                return;
            }

            auto ttsResp = self->ParseResponse(resp, 200);
            self->available_ = ttsResp.success;
            if (!ttsResp.success) {
                self->lastError_ = ttsResp.errorMsg;
            }
            try {
                callback({ ttsResp.audioData, ttsResp.success, ttsResp.errorMsg });
            } catch (...) {
            }
        });
}

std::string XiaomiTTSProvider::BuildRequestBody(const TTSRequest& request) const {
    const auto& config = ConfigManager::Inst()->GetConfig();
    std::string processedText = HashtagToStyle(request.text);
    std::string styleTag = config.mimoStyle.empty() ? "" : "<style>" + config.mimoStyle + "</style>";
    std::string fullText = styleTag + processedText;

    nlohmann::json j;
    j["model"] = "mimo-v2-tts";
    j["messages"] = {
        {{"role", "assistant"}, {"content", fullText}}
    };
    json audio;
    audio["voice"] = config.mimoVoice.empty() ? "mimo_default" : config.mimoVoice;
    audio["format"] = "mp3";
    j["audio"] = audio;
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
    unsigned char char_array_4[4] = {0, 0, 0, 0};
    unsigned char char_array_3[3] = {0, 0, 0};

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

std::string XiaomiTTSProvider::HashtagToStyle(const std::string& input) const {
    std::string result = input;
    std::regex pattern(R"(#([^#]+)#)");
    std::smatch match;
    
    // Step 1: 将第一个标签移到最前面，避免API忽略标签前的文本
    if (std::regex_search(result, match, pattern)) {
        std::string tag = match[0].str(); // 完整的 #标签# 文本
        result.erase(match.position(), match[0].length());
        result = tag + result;
    }
    
    // Step 2: 替换所有 #标签# 为 <style>标签</style>
    size_t searchPos = 0;
    while (searchPos < result.length()) {
        if (!std::regex_search(result.cbegin() + searchPos, result.cend(), match, pattern)) {
            break;
        }
        size_t matchStart = searchPos + match.position();
        std::string tag = match[1].str();
        std::string replacement = "<style>" + tag + "</style>";
        result.replace(result.begin() + matchStart, result.begin() + matchStart + match[0].length(), replacement);
        searchPos = matchStart + replacement.length();
    }
    return result;
}
