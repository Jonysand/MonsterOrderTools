#include "framework.h"
#include "MiniMaxAIChatProvider.h"
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

MiniMaxAIChatProvider::MiniMaxAIChatProvider(const std::string& apiKey)
    : apiKey_(apiKey), available_(false) {}

std::string MiniMaxAIChatProvider::GetProviderName() const { return "minimax"; }

bool MiniMaxAIChatProvider::IsAvailable() const { return available_; }

std::string MiniMaxAIChatProvider::GetLastError() const { return lastError_; }

bool MiniMaxAIChatProvider::CallAPI(const std::string& prompt, std::string& outResponse) {
    std::string body = R"({
        "model": "M2-her",
        "messages": [{"role": "user", "content": ")" + prompt + R"("}]
    })";

    std::string headersStr =
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey_ + "\r\n";

    if (!MakeSyncHttpsRequest(
        "api.minimaxi.com",
        443,
        "/v1/text/chatcompletion_v2",
        headersStr,
        body,
        outResponse)) {
        lastError_ = "HTTP request failed";
        available_ = false;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(outResponse);
        outResponse = j["choices"][0]["message"]["content"].get<std::string>();
        available_ = true;
        return true;
    }
    catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        available_ = false;
        return false;
    }
}

bool MiniMaxAIChatProvider::MakeSyncHttpsRequest(
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
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        std::wstring(host.begin(), host.end()).c_str(),
        (INTERNET_PORT)port,
        0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
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
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
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
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bResults = WinHttpReceiveResponse(hRequest, nullptr);
    if (!bResults) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
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
    return true;
}
