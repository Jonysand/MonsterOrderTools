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
        response.audioData = {};
        callback(response);
    }
    else {
        TTSResponse response;
        response.success = false;
        response.errorMsg = "SAPI speak failed";
        callback(response);
    }
}
