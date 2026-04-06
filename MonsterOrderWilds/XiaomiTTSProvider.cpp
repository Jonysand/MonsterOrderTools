#include "framework.h"
#include "ITTSProvider.h"
#include "TTSProvider.h"
#include "MimoTTSClient.h"
#include <winhttp.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")

XiaomiTTSProvider::XiaomiTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string XiaomiTTSProvider::GetProviderName() const { return "xiaomi"; }

bool XiaomiTTSProvider::IsAvailable() const { return available_; }

std::string XiaomiTTSProvider::GetLastError() const { return lastError_; }

void XiaomiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    std::string responseBody;
    if (!MakeSyncHttpsRequest("api.xiaomimimo.com", 443, "/v1/chat/completions", headers, body, responseBody)) {
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

bool XiaomiTTSProvider::MakeSyncHttpsRequest(
    const std::string& host, int port, const std::string& path,
    const std::string& headers, const std::string& body,
    std::string& outResponse) {
    outResponse.clear();

    HINTERNET hSession = WinHttpOpen(L"MonsterOrderWilds/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession,
        std::wstring(host.begin(), host.end()).c_str(),
        (INTERNET_PORT)port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
        std::wstring(path.begin(), path.end()).c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::wstring wHeaders(headers.begin(), headers.end());
    if (!WinHttpSendRequest(hRequest, wHeaders.c_str(), (DWORD)wHeaders.length(),
        (LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
    }

    std::string responseData;
    DWORD dwSize = 0;
    while (true) {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
        std::vector<char> buffer(dwSize + 1);
        DWORD dwRead = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwRead)) break;
        buffer[dwRead] = '\0';
        responseData.append(buffer.data(), dwRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    outResponse = responseData;
    return true;
}