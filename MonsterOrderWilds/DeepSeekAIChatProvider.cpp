#include "framework.h"
#include "DeepSeekAIChatProvider.h"
#include "Network.h"
#include <winhttp.h>
#include <string>
#include <vector>
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

DeepSeekAIChatProvider::DeepSeekAIChatProvider(const std::string& apiKey)
    : apiKey_(apiKey), available_(false) {}

std::string DeepSeekAIChatProvider::GetProviderName() const { return "deepseek"; }



std::string DeepSeekAIChatProvider::GetLastError() const { return lastError_; }

std::string DeepSeekAIChatProvider::BuildRequestBody(const std::string& prompt) {
    // 依据 DeepSeek 官方文档《思考模式》：
    // https://api-docs.deepseek.com/zh-cn/guides/thinking_mode/
    // - 思考模式开关（OpenAI 格式）："thinking": {"type": "enabled"}（默认即开启，此处显式声明）
    // - 思考强度控制：       "reasoning_effort": "low"（low/high/max）
    // 模型使用 deepseek-v4-flash，思考模式与其参数均按 V4 文档校验。
    nlohmann::json reqBody;
    reqBody["model"] = "deepseek-v4-flash";
    reqBody["thinking"] = {{"type", "enabled"}};
    reqBody["reasoning_effort"] = "low";
    reqBody["messages"] = nlohmann::json::array();
    reqBody["messages"].push_back({{"role", "user"}, {"content", prompt}});
    return reqBody.dump();
}

bool DeepSeekAIChatProvider::CallAPI(const std::string& prompt, std::string& outResponse) {
    std::string body = BuildRequestBody(prompt);

    std::string headersStr =
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey_ + "\r\n";

    std::string response;
    DWORD httpError = 0;
    DWORD httpStatusCode = 0;
    std::mutex mtx;
    std::condition_variable cv;
    bool completed = false;

    Network::MakeHttpsRequestAsync(
        TEXT("api.deepseek.com"),
        443,
        TEXT("/chat/completions"),
        TEXT("POST"),
        headersStr,
        body,
        true,
        [&](bool success, const std::string& resp, DWORD error, DWORD statusCode) {
            try {
                std::lock_guard<std::mutex> lock(mtx);
                response = resp;
                httpError = error;
                httpStatusCode = statusCode;
                completed = true;
                cv.notify_one();
            } catch (...) {
                // Mutex operations should not throw, but handle defensively
            }
        });

    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&completed]() { return completed; });

    if (httpError != 0) {
        lastError_ = "HTTP request failed: " + GetWinHttpErrorString(httpError);
        available_ = false;
        return false;
    }

    if (httpStatusCode != 200) {
        lastError_ = "HTTP error: " + std::to_string(httpStatusCode);
        if (!response.empty()) {
            lastError_ += ", response: " + response.substr(0, 200);
        }
        available_ = false;
        return false;
    }

    outResponse = response;
    try {
        auto responseJson = nlohmann::json::parse(outResponse);
        if (!responseJson.contains("choices") || !responseJson["choices"].is_array() || responseJson["choices"].empty()) {
            lastError_ = "No choices in response";
            available_ = false;
            return false;
        }
        auto& choice = responseJson["choices"][0];
        if (!choice.contains("message")) {
            lastError_ = "No message in response";
            available_ = false;
            return false;
        }
        auto& message = choice["message"];
        // 思考模式下思维链通过 reasoning_content 返回，最终回答在 content。
        // 优先取最终回答；content 为空时兜底取 reasoning_content。
        std::string content;
        if (message.contains("content") && message["content"].is_string()) {
            content = message["content"].get<std::string>();
        }
        if (content.empty() && message.contains("reasoning_content") && message["reasoning_content"].is_string()) {
            content = message["reasoning_content"].get<std::string>();
        }
        if (content.empty()) {
            lastError_ = "No message content in response";
            available_ = false;
            return false;
        }
        outResponse = content;
        available_ = true;
        return true;
    }
    catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        available_ = false;
        return false;
    }
}

void DeepSeekAIChatProvider::CallAPIAsync(const std::string& prompt, std::function<void(bool, const std::string&)> callback) {
    std::string body = BuildRequestBody(prompt);

    std::string headersStr =
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey_ + "\r\n";

    Network::MakeHttpsRequestAsync(
        TEXT("api.deepseek.com"),
        443,
        TEXT("/chat/completions"),
        TEXT("POST"),
        headersStr,
        body,
        true,
        [this, callback](bool success, const std::string& resp, DWORD error, DWORD httpStatusCode) {
            if (!success || error != 0 || httpStatusCode != 200) {
                lastError_ = "HTTP request failed";
                if (error != 0) {
                    lastError_ += ": " + GetWinHttpErrorString(error);
                } else if (httpStatusCode != 0 && httpStatusCode != 200) {
                    lastError_ += " (HTTP " + std::to_string(httpStatusCode) + ")";
                }
                available_ = false;
                if (callback) callback(false, "");
                return;
            }

            std::string response = resp;
            try {
                auto responseJson = nlohmann::json::parse(response);
                if (!responseJson.contains("choices") || !responseJson["choices"].is_array() || responseJson["choices"].empty()) {
                    lastError_ = "No choices in response";
                    available_ = false;
                    if (callback) callback(false, "");
                    return;
                }
                auto& choice = responseJson["choices"][0];
                if (!choice.contains("message")) {
                    lastError_ = "No message in response";
                    available_ = false;
                    if (callback) callback(false, "");
                    return;
                }
                auto& message = choice["message"];
                // 思考模式下思维链通过 reasoning_content 返回，最终回答在 content。
                // 优先取最终回答；content 为空时兜底取 reasoning_content。
                std::string content;
                if (message.contains("content") && message["content"].is_string()) {
                    content = message["content"].get<std::string>();
                }
                if (content.empty() && message.contains("reasoning_content") && message["reasoning_content"].is_string()) {
                    content = message["reasoning_content"].get<std::string>();
                }
                if (content.empty()) {
                    lastError_ = "No message content in response";
                    available_ = false;
                    if (callback) callback(false, "");
                    return;
                }
                available_ = true;
                if (callback) callback(true, content);
            }
            catch (const std::exception& e) {
                lastError_ = std::string("JSON parse error: ") + e.what();
                available_ = false;
                if (callback) callback(false, "");
            }
        });
}
