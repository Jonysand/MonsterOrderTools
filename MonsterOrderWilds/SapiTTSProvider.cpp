#include "framework.h"
#include "ITTSProvider.h"
#include "TTSProvider.h"
#include "TextToSpeech.h"
#include "StringProcessor.h"

SapiTTSProvider::SapiTTSProvider() = default;

std::string SapiTTSProvider::GetProviderName() const { return "sapi"; }

bool SapiTTSProvider::IsAvailable() const { return true; }

std::string SapiTTSProvider::GetLastError() const { return lastError_; }

void SapiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    std::wstring wtext = StringProcessor::Utf8ToWstring(request.text);

    if (TTSManager::Inst()->SpeakWithSapi(wtext)) {
        TTSResponse response;
        response.success = true;
        response.audioData = {1};  // SAPI直接播报，无需音频数据，填充标志位避免被误判为失败
        callback(response);
    }
    else {
        TTSResponse response;
        response.success = false;
        response.errorMsg = "SAPI speak failed";
        callback(response);
    }
}
