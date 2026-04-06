#include "framework.h"
#include "TTSProvider.h"
#include "WriteLog.h"

std::unique_ptr<ITTSProvider> TTSProviderFactory::Create(
    const std::string& credentialJson) {
    try {
        auto cred = nlohmann::json::parse(credentialJson);

        std::string provider = cred.value("tts_provider", "xiaomi");
        std::string apiKey = cred.value("tts_api_key", "");

        if (apiKey.empty()) {
            LOG_INFO(TEXT("TTS API key is empty, falling back to SAPI"));
            return std::make_unique<SapiTTSProvider>();
        }

        if (provider == "xiaomi") {
            return std::make_unique<XiaomiTTSProvider>(apiKey);
        }
        else if (provider == "minimax") {
            return std::make_unique<MiniMaxTTSProvider>(apiKey);
        }

        LOG_WARNING(TEXT("Unsupported TTS provider: %hs, falling back to SAPI"), provider.c_str());
        return std::make_unique<SapiTTSProvider>();
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("Failed to create TTS provider: %hs"), e.what());
        return std::make_unique<SapiTTSProvider>();
    }
}