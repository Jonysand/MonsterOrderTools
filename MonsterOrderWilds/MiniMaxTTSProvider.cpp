#include "framework.h"
#include "TTSProvider.h"
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winhttp.lib")

MiniMaxTTSProvider::MiniMaxTTSProvider(const std::string& apiKey) : apiKey_(apiKey), available_(false) {}

std::string MiniMaxTTSProvider::GetProviderName() const { return "minimax"; }

bool MiniMaxTTSProvider::IsAvailable() const { return available_; }

std::string MiniMaxTTSProvider::GetLastError() const { return lastError_; }

void MiniMaxTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string body = BuildRequestBody(request);
    std::string headers = BuildRequestHeaders(apiKey_);

    std::string responseBody;
    if (!MakeSyncHttpsRequest("api.minimaxi.com", 443, "/v1/t2a_v2", headers, body, responseBody)) {
        lastError_ = "HTTP request failed";
        available_ = false;
        TTSResponse resp;
        resp.success = false;
        resp.errorMsg = lastError_;
        callback(resp);
        return;
    }

    auto resp = ParseResponse(responseBody);
    available_ = resp.audioData.size() > 0;
    if (!available_) {
        lastError_ = resp.errorMsg;
    }
    callback(resp);
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
            result.errorMsg = j["error"]["message"].get<std::string>();
            return result;
        }

        if (j.contains("data") && j["data"].contains("audio")) {
            std::string audioHex = j["data"]["audio"].get<std::string>();
            result.audioData = HexToBytes(audioHex);
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

std::vector<uint8_t> MiniMaxTTSProvider::HexToBytes(const std::string& hex) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

bool MiniMaxTTSProvider::MakeSyncHttpsRequest(
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
