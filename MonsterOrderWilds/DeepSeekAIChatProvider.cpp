#include "framework.h"
#include "DeepSeekAIChatProvider.h"
#include <winhttp.h>
#include <string>
#include <vector>

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

bool DeepSeekAIChatProvider::IsAvailable() const { return available_; }

std::string DeepSeekAIChatProvider::GetLastError() const { return lastError_; }

bool DeepSeekAIChatProvider::CallAPI(const std::string& prompt, std::string& outResponse) {
    nlohmann::json reqBody;
    reqBody["model"] = "deepseek-chat";
    reqBody["messages"] = nlohmann::json::array();
    reqBody["messages"].push_back({{"role", "user"}, {"content", prompt}});
    std::string body = reqBody.dump();

    std::string headersStr =
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey_ + "\r\n";

    DWORD httpError = MakeSyncHttpsRequest(
        "api.deepseek.com",
        443,
        "/chat/completions",
        headersStr,
        body,
        outResponse);
    if (httpError != 0) {
        lastError_ = "HTTP request failed: " + GetWinHttpErrorString(httpError);
        available_ = false;
        return false;
    }

    try {
        auto responseJson = nlohmann::json::parse(outResponse);
        outResponse = responseJson["choices"][0]["message"]["content"].get<std::string>();
        available_ = true;
        return true;
    }
    catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        available_ = false;
        return false;
    }
}

DWORD DeepSeekAIChatProvider::MakeSyncHttpsRequest(
    const std::string& host,
    int port,
    const std::string& path,
    const std::string& headers,
    const std::string& body,
    std::string& outResponse) {
    outResponse.clear();

    HINTERNET hSession = WinHttpOpen(
        L"MonsterOrderWilds/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) return ::GetLastError();

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        std::wstring(host.begin(), host.end()).c_str(),
        (INTERNET_PORT)port,
        0);
    if (!hConnect) {
        DWORD err = ::GetLastError();
        WinHttpCloseHandle(hSession);
        return err;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        std::wstring(path.begin(), path.end()).c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        DWORD err = ::GetLastError();
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return err;
    }

    std::wstring wHeaders(headers.begin(), headers.end());
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        wHeaders.c_str(),
        (DWORD)wHeaders.length(),
        (LPVOID)body.c_str(),
        (DWORD)body.length(),
        (DWORD)body.length(),
        0);

    if (!bResults) {
        DWORD err = ::GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return err;
    }

    bResults = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResults) {
        DWORD err = ::GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return err;
    }

    DWORD dwSize = 0;
    std::string responseData;
    while (true) {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0)
            break;

        std::vector<char> buffer(dwSize + 1);
        DWORD dwRead = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwRead))
            break;

        buffer[dwRead] = '\0';
        responseData.append(buffer.data(), dwRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    outResponse = responseData;
    return 0;
}