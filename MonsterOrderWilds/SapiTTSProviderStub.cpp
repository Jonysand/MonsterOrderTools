#include "framework.h"
#include "TTSProvider.h"

#ifdef RUN_UNIT_TESTS

SapiTTSProvider::SapiTTSProvider() = default;

std::string SapiTTSProvider::GetProviderName() const {
    return "sapi";
}

bool SapiTTSProvider::IsAvailable() const {
    return true;
}

std::string SapiTTSProvider::GetLastError() const {
    return "";
}

void SapiTTSProvider::RequestTTS(const TTSRequest& request, TTSCallback callback) {
    TTSResponse response;
    response.success = true;
    response.audioData = {1, 2, 3, 4};
    response.errorMsg = "";
    callback(response);
}

#endif
