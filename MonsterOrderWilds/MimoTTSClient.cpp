#include "MimoTTSClient.h"
#include "WriteLog.h"
#include "CredentialsManager.h"
#include "StringUtils.h"
#include "Network.h"
#include <mutex>
#include <condition_variable>

MimoTTSClient::MimoTTSClient()
{
}

MimoTTSClient::~MimoTTSClient()
{
}

void MimoTTSClient::RequestTTS(const TTSRequest& request, TTSCallback callback)
{
#if USE_MIMO_TTS
    ExecuteWithRetry(request, callback, 3);
#else
    TTSResponse response;
    response.success = false;
    response.errorMessage = "MiMo TTS is disabled by compile flag";
    if (callback) {
        callback(response);
    }
#endif
}

bool MimoTTSClient::IsAvailable() const
{
#if USE_MIMO_TTS
    return !GetMIMO_API_KEY().empty();
#else
    return false;
#endif
}

std::string MimoTTSClient::GetLastError() const
{
    return lastError;
}

std::string MimoTTSClient::BuildRequestBody(const TTSRequest& request) const
{
    json body;
    body["model"] = request.model;
    
    json messages = json::array();
    
    json assistantMsg;
    assistantMsg["role"] = "assistant";
    assistantMsg["content"] = request.text;
    messages.push_back(assistantMsg);
    
    body["messages"] = messages;
    body["modalities"] = json::array({"text", "audio"});
    
    json audio;
    audio["voice"] = request.voice;
    audio["format"] = request.responseFormat;
    if (request.speed != 1.0f) {
        audio["speed"] = request.speed;
    }
    body["audio"] = audio;

    std::string result = body.dump();
    LOG_INFO(TEXT("MiMo TTS Request Body: %s"), utf8_to_wstring(result).c_str());
    return result;
}

std::string MimoTTSClient::BuildRequestHeaders(const std::string& apiKey) const
{
    std::string headers;
    headers += "Content-Type: application/json\r\n";
    headers += "Authorization: Bearer " + apiKey + "\r\n";
    return headers;
}

MimoTTSClient::TTSResponse MimoTTSClient::ParseResponse(const std::string& responseBody, int httpStatusCode)
{
    TTSResponse response;
    response.httpStatusCode = httpStatusCode;

    if (httpStatusCode == 200) {
        try {
            json resultJson = json::parse(responseBody);
            LOG_INFO(TEXT("MiMo TTS Response parsed successfully"));
            
            if (resultJson.contains("choices") && resultJson["choices"].is_array() && !resultJson["choices"].empty()) {
                auto& choice = resultJson["choices"][0];
                if (choice.is_object() && choice.contains("message") && choice["message"].contains("audio")) {
                    auto& audio = choice["message"]["audio"];
                    if (audio.contains("data") && audio["data"].is_string()) {
                        std::string base64Audio = audio["data"].get<std::string>();
                        LOG_INFO(TEXT("MiMo TTS Audio data length: %d"), base64Audio.length());
                        
                        static const std::string base64_chars = 
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                        
                        std::vector<uint8_t> decoded;
                        int in_len = base64Audio.size();
                        int i = 0, j = 0, in_ = 0;
                        uint8_t char_array_4[4], char_array_3[3];
                        
                        while (in_len-- && (base64Audio[in_] != '=') && 
                               (isalnum(base64Audio[in_]) || (base64Audio[in_] == '+') || (base64Audio[in_] == '/'))) {
                            char_array_4[i++] = base64Audio[in_]; in_++;
                            if (i == 4) {
                                for (i = 0; i <4; i++)
                                    char_array_4[i] = base64_chars.find(char_array_4[i]);
                                
                                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                                
                                for (i = 0; (i < 3); i++)
                                    decoded.push_back(char_array_3[i]);
                                i = 0;
                            }
                        }
                        
                        if (i) {
                            for (j = 0; j < i; j++)
                                char_array_4[j] = base64_chars.find(char_array_4[j]);
                            
                            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                            
                            for (j = 0; (j < i - 1); j++)
                                decoded.push_back(char_array_3[j]);
                        }
                        
                        response.success = true;
                        response.audioData = decoded;
                        return response;
                    }
                }
            }
            
            response.success = false;
            response.errorMessage = "No audio data in response";
            
        } catch (const std::exception& e) {
            response.success = false;
            response.errorMessage = std::string("Failed to parse response: ") + e.what();
        }
    } else {
        response.success = false;
        try {
            json errorJson = json::parse(responseBody);
            if (errorJson.contains("error")) {
                if (errorJson["error"].is_string()) {
                    response.errorMessage = errorJson["error"].get<std::string>();
                } else if (errorJson["error"].contains("message") && errorJson["error"]["message"].is_string()) {
                    response.errorMessage = errorJson["error"]["message"].get<std::string>();
                }
            }
        } catch (...) {
            response.errorMessage = "Unknown error (HTTP " + std::to_string(httpStatusCode) + ")";
        }
    }

    return response;
}

void MimoTTSClient::ExecuteWithRetry(const TTSRequest& request, TTSCallback callback, int maxRetries)
{
#if USE_MIMO_TTS
    std::string apiKey = GetMIMO_API_KEY();
    if (apiKey.empty()) {
        lastError = "API Key not configured";
        TTSResponse response;
        response.success = false;
        response.errorMessage = lastError;
        if (callback) {
            callback(response);
        }
        return;
    }

    std::string requestBody = BuildRequestBody(request);
    std::string requestHeaders = BuildRequestHeaders(apiKey);

    bool useSSL = true;

    auto callbackPtr = std::make_shared<std::function<void(const TTSResponse&)>>(callback);
    auto requestBodyCopy = requestBody;
    auto requestHeadersCopy = requestHeaders;
    struct RetryContext {
        int retryCount = 0;
        std::mutex mtx;
        std::condition_variable cv;
        bool completed = false;
        TTSResponse finalResponse;
    };
    auto ctx = std::make_shared<RetryContext>();

    std::shared_ptr<std::function<void(bool, const std::string&, DWORD)>> requestWithRetryPtr;
    requestWithRetryPtr = std::make_shared<std::function<void(bool, const std::string&, DWORD)>>(
        [requestWithRetryPtr, callbackPtr, requestBodyCopy, requestHeadersCopy, useSSL, maxRetries, ctx](bool success, const std::string& responseBody, DWORD error) {
        try {
            TTSResponse response;
            if (success && !responseBody.empty()) {
                response = ParseResponse(responseBody, 200);
            } else {
                response.success = false;
                response.errorMessage = error != 0 ? "HTTP error: " + std::to_string(error) : "Empty response from server";
            }
            response.retryCount = ctx->retryCount;

            if (!response.success && ctx->retryCount < maxRetries) {
                ctx->retryCount++;
                LOG_ERROR(TEXT("MiMo TTS request failed, retrying (%d/%d)..."), ctx->retryCount, maxRetries);

                Network::MakeHttpsRequestAsync(
                    utf8_to_wstring(API_ENDPOINT),
                    API_PORT,
                    utf8_to_wstring(API_PATH),
                    TEXT("POST"),
                    requestHeadersCopy,
                    requestBodyCopy,
                    useSSL,
                    *requestWithRetryPtr
                );
            } else {
                if (!response.success) {
                    LOG_ERROR(TEXT("MiMo TTS request failed after %d retries: %s"),
                        maxRetries, utf8_to_wstring(response.errorMessage).c_str());
                }
                std::lock_guard<std::mutex> lock(ctx->mtx);
                ctx->finalResponse = response;
                ctx->completed = true;
                ctx->cv.notify_one();
            }
        } catch (...) {
            LOG_ERROR(TEXT("MimoTTSClient retry callback exception"));
        }
    });

    Network::MakeHttpsRequestAsync(
        utf8_to_wstring(API_ENDPOINT),
        API_PORT,
        utf8_to_wstring(API_PATH),
        TEXT("POST"),
        requestHeadersCopy,
        requestBodyCopy,
        useSSL,
        *requestWithRetryPtr
    );

    std::unique_lock<std::mutex> lock(ctx->mtx);
    ctx->cv.wait(lock, [&ctx]() { return ctx->completed; });

    if (*callbackPtr) {
        try {
            (*callbackPtr)(ctx->finalResponse);
        } catch (...) {
            LOG_ERROR(TEXT("[MimoTTSClient] TTS callback exception"));
        }
    }
#endif
}
