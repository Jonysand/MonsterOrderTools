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
            std::lock_guard<std::mutex> lock(mtx);
            responseBody = resp;
            httpError = error;
            completed = true;
            cv.notify_one();
        });

    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&completed]() { return completed; });

    if (httpError != 0) {
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
