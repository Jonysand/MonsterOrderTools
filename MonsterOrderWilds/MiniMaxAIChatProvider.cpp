#include "framework.h"
#include "MiniMaxAIChatProvider.h"
#include "Network.h"
#include <winhttp.h>
#include <string>
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

MiniMaxAIChatProvider::MiniMaxAIChatProvider(const std::string& apiKey)
    : apiKey_(apiKey), available_(false) {}

std::string MiniMaxAIChatProvider::GetProviderName() const { return "minimax"; }

bool MiniMaxAIChatProvider::IsAvailable() const { return available_; }

std::string MiniMaxAIChatProvider::GetLastError() const { return lastError_; }

bool MiniMaxAIChatProvider::CallAPI(const std::string& prompt, std::string& outResponse) {
    nlohmann::json requestBody;
    requestBody["model"] = "M2-her";
    requestBody["messages"] = nlohmann::json::array();
    requestBody["messages"].push_back({{"role", "user"}, {"content", prompt}});
    std::string body = requestBody.dump();

    std::string headersStr =
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey_ + "\r\n";

    std::string response;
    DWORD httpError = 0;
    std::mutex mtx;
    std::condition_variable cv;
    bool completed = false;

    Network::MakeHttpsRequestAsync(
        TEXT("api.minimaxi.com"),
        443,
        TEXT("/v1/text/chatcompletion_v2"),
        TEXT("POST"),
        headersStr,
        body,
        true,
        [&](bool success, const std::string& resp, DWORD error) {
            try {
                std::lock_guard<std::mutex> lock(mtx);
                response = resp;
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
        lastError_ = "HTTP request failed: " + GetWinHttpErrorString(httpError);
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
        if (!choice.contains("message") || !choice["message"].contains("content")) {
            lastError_ = "No message content in response";
            available_ = false;
            return false;
        }
        outResponse = choice["message"]["content"].get<std::string>();
        available_ = true;
        return true;
    }
    catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        available_ = false;
        return false;
    }
}
