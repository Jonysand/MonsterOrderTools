#include "framework.h"
#include "TTSProvider.h"
#include "Network.h"
#include "WriteLog.h"
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winhttp.lib")

namespace {
    std::string UrlEncode(const std::string& value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;
        
        for (char c : value) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
                escaped << std::nouppercase;
            }
        }
        
        return escaped.str();
    }
}

ManboTTSProvider::ManboTTSProvider() : available_(true) {}

std::string ManboTTSProvider::GetProviderName() const { return "manbo"; }

std::string ManboTTSProvider::GetLastError() const { return lastError_; }

std::string ManboTTSProvider::BuildRequestUrl(const TTSRequest& request) const {
    std::string url = "/api/speech/AiChat/?module=audio&text=";
    url += UrlEncode(request.text);
    url += "&voice=";
    url += UrlEncode("曼波");
    return url;
}

TTSResponse ManboTTSProvider::ParseApiResponse(const std::string& responseBody) const {
    TTSResponse result;
    result.success = false;
    
    try {
        auto j = nlohmann::json::parse(responseBody);
        if (j.contains("code") && j["code"].get<int>() == 200) {
            if (j.contains("data") && j["data"].contains("audio_url") && j["data"]["audio_url"].is_string()) {
                result.success = true;
                result.errorMsg = j["data"]["audio_url"].get<std::string>();
            } else {
                result.errorMsg = "Invalid response format: missing audio_url";
            }
        } else {
            if (j.contains("message") && j["message"].is_string()) {
                result.errorMsg = j["message"].get<std::string>();
            } else {
                result.errorMsg = "API error";
            }
        }
    } catch (const std::exception& e) {
        result.errorMsg = std::string("Parse error: ") + e.what();
    }
    return result;
}

void ManboTTSProvider::DownloadAudio(const std::string& audioUrl, TTSCallback callback) {
    std::string host;
    std::string path;
    int port = 443;
    bool useHttps = true;
    
    size_t protocolEnd = audioUrl.find("://");
    if (protocolEnd != std::string::npos) {
        useHttps = audioUrl.substr(0, protocolEnd) == "https";
        size_t hostStart = protocolEnd + 3;
        size_t pathStart = audioUrl.find('/', hostStart);
        if (pathStart != std::string::npos) {
            host = audioUrl.substr(hostStart, pathStart - hostStart);
            path = audioUrl.substr(pathStart);
        } else {
            host = audioUrl.substr(hostStart);
            path = "/";
        }
    } else {
        lastError_ = "Invalid audio URL format";
        TTSResponse resp;
        resp.success = false;
        resp.errorMsg = lastError_;
        try {
            callback(resp);
        } catch (...) {}
        return;
    }
    
    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());
    
    Network::MakeHttpsRequestAsync(
        wHost.c_str(),
        port,
        wPath.c_str(),
        TEXT("GET"),
        "",
        "",
        useHttps,
        [this, callback, audioUrl](bool success, const std::string& resp, DWORD error) {
            if (!success || error != 0) {
                lastError_ = "Audio download failed";
                available_ = false;
                TTSResponse response;
                response.success = false;
                response.errorMsg = lastError_;
                try {
                    callback(response);
                } catch (...) {}
                return;
            }

            TTSResponse response;
            response.success = true;
            response.audioData.assign(resp.begin(), resp.end());

            size_t dotPos = audioUrl.rfind('.');
            size_t queryPos = audioUrl.find('?');
            if (dotPos != std::string::npos && dotPos < audioUrl.size() - 1) {
                std::string ext;
                if (queryPos == std::string::npos || queryPos > dotPos) {
                    ext = audioUrl.substr(dotPos + 1);
                } else {
                    ext = audioUrl.substr(dotPos + 1, queryPos - dotPos - 1);
                }
                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                if (!ext.empty() && ext.find('/') == std::string::npos) {
                    response.format = ext;
                }
            }

            try {
                callback(response);
            } catch (...) {}
        });
}

void ManboTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::string requestUrl = BuildRequestUrl(request);
    std::wstring wRequestUrl(requestUrl.begin(), requestUrl.end());
    
    Network::MakeHttpsRequestAsync(
        TEXT("api-v2.cenguigui.cn"),
        443,
        wRequestUrl.c_str(),
        TEXT("GET"),
        "",
        "",
        true,
        [this, callback](bool success, const std::string& resp, DWORD error) {
            if (!success || error != 0) {
                lastError_ = "HTTP request failed";
                available_ = false;
                TTSResponse response;
                response.success = false;
                response.errorMsg = lastError_;
                try {
                    callback(response);
                } catch (...) {}
                return;
            }
            
            auto apiResp = ParseApiResponse(resp);
            if (!apiResp.success) {
                lastError_ = apiResp.errorMsg;
                available_ = false;
                TTSResponse response;
                response.success = false;
                response.errorMsg = lastError_;
                try {
                    callback(response);
                } catch (...) {}
                return;
            }
            
            std::string audioUrl = apiResp.errorMsg;
            DownloadAudio(audioUrl, callback);
        });
}
