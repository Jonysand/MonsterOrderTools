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

XiaomiTTSProvider::XiaomiTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string XiaomiTTSProvider::GetProviderName() const { return "xiaomi"; }



std::string XiaomiTTSProvider::GetLastError() const { return lastError_; }

void XiaomiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    Network::MakeHttpsRequestAsync(
        TEXT("token-plan-cn.xiaomimimo.com"),
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
    std::string processedText = ProcessStyleTags(request.text);

    nlohmann::json j;
    j["model"] = "mimo-v2.5-tts";
    j["messages"] = {
        {{"role", "assistant"}, {"content", processedText}}
    };
    json audio;
    audio["voice"] = config.mimoVoice.empty() ? "mimo_default" : config.mimoVoice;
    audio["format"] = "mp3";
    j["audio"] = audio;
    return j.dump();
}

std::string XiaomiTTSProvider::BuildRequestHeaders(const std::string& apiKey) const {
    return "Content-Type: application/json\r\napi-key: " + apiKey + "\r\n";
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

namespace {
    std::string Trim(const std::string& str) {
        size_t start = 0;
        while (start < str.size() && (str[start] == ' ' || str[start] == '\t')) {
            ++start;
        }
        size_t end = str.size();
        while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
            --end;
        }
        return str.substr(start, end - start);
    }

    std::vector<std::string> SplitStyles(const std::string& content) {
        std::vector<std::string> result;
        std::string current;
        for (size_t i = 0; i < content.size(); ) {
            char32_t ch = 0;
            // 解析UTF-8字符
            unsigned char c = static_cast<unsigned char>(content[i]);
            if (c < 0x80) {
                ch = c;
                i += 1;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < content.size()) {
                ch = ((c & 0x1F) << 6) | (content[i + 1] & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < content.size()) {
                ch = ((c & 0x0F) << 12) | ((content[i + 1] & 0x3F) << 6) | (content[i + 2] & 0x3F);
                i += 3;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < content.size()) {
                ch = ((static_cast<char32_t>(c) & 0x07) << 18) | ((content[i + 1] & 0x3F) << 12) | ((content[i + 2] & 0x3F) << 6) | (content[i + 3] & 0x3F);
                i += 4;
            } else {
                // 非法UTF-8，跳过
                i += 1;
                continue;
            }

            // 检查是否是分隔符：逗号、顿号、空格、竖线、分号
            if (ch == U',' || ch == U'，' || ch == U' ' || ch == U'\t' || ch == U'|' || ch == U';' || ch == U'；') {
                std::string trimmed = Trim(current);
                if (!trimmed.empty()) {
                    result.push_back(trimmed);
                }
                current.clear();
            } else {
                // 将字符加回current（需要重新编码为UTF-8）
                if (ch < 0x80) {
                    current += static_cast<char>(ch);
                } else if (ch < 0x800) {
                    current += static_cast<char>(0xC0 | (ch >> 6));
                    current += static_cast<char>(0x80 | (ch & 0x3F));
                } else if (ch < 0x10000) {
                    current += static_cast<char>(0xE0 | (ch >> 12));
                    current += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    current += static_cast<char>(0x80 | (ch & 0x3F));
                } else {
                    current += static_cast<char>(0xF0 | (ch >> 18));
                    current += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                    current += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    current += static_cast<char>(0x80 | (ch & 0x3F));
                }
            }
        }
        // 处理最后一个
        std::string trimmed = Trim(current);
        if (!trimmed.empty()) {
            result.push_back(trimmed);
        }
        return result;
    }
}

std::string XiaomiTTSProvider::ProcessStyleTags(const std::string& input) const {
    std::string result = input;

    // 匹配所有风格标签: (内容), （内容）, [内容]
    // 注意：不使用 \u 转义，直接嵌入 UTF-8 字符，避免 std::regex 解析异常
    std::regex pattern(R"((?:\(|（|\[)([^)\]）]+)(?:\)|）|\]))");
    std::smatch match;

    std::vector<std::string> styles;

    // 提取所有风格标签并解析内部风格
    std::string temp = result;
    while (std::regex_search(temp, match, pattern)) {
        std::string content = match[1].str();
        std::vector<std::string> innerStyles = SplitStyles(content);
        for (const auto& s : innerStyles) {
            if (!s.empty()) {
                styles.push_back(s);
            }
        }
        temp = match.suffix().str();
    }

    // 如果没有风格标签，直接返回
    if (styles.empty()) {
        return result;
    }

    // 移除所有风格标签标记
    result = std::regex_replace(result, pattern, "");
    // 清理多余空格
    result = Trim(result);

    // 合并所有风格，使用逗号分隔
    std::string stylePrefix = "(";
    for (size_t i = 0; i < styles.size(); ++i) {
        if (i > 0) stylePrefix += ",";
        stylePrefix += styles[i];
    }
    stylePrefix += ")";

    // 将风格标签放在开头
    return stylePrefix + result;
}
