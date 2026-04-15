#include "framework.h"
#include "TTSProvider.h"
#include "WriteLog.h"

std::unique_ptr<ITTSProvider> TTSProviderFactory::Create(
    const std::string& mimoApiKey,
    const std::string& minimaxApiKey,
    const std::string& ttsEngine) {

    if (ttsEngine == "sapi") {
        LOG_INFO(TEXT("TTS engine set to SAPI"));
        return std::make_unique<SapiTTSProvider>();
    }

    if (ttsEngine == "mimo") {
        if (mimoApiKey.empty()) {
            LOG_WARNING(TEXT("MiMo TTS API key is empty, falling back to SAPI"));
            return std::make_unique<SapiTTSProvider>();
        }
        LOG_INFO(TEXT("TTS engine set to MiMo (explicit)"));
        return std::make_unique<XiaomiTTSProvider>(mimoApiKey);
    }

    // AUTO mode: minimax -> xiaomi -> sapi
    if (!minimaxApiKey.empty()) {
        LOG_INFO(TEXT("TTS engine: Auto -> MiniMax"));
        return std::make_unique<MiniMaxTTSProvider>(minimaxApiKey);
    }

    if (!mimoApiKey.empty()) {
        LOG_INFO(TEXT("TTS engine: Auto -> MiMo"));
        return std::make_unique<XiaomiTTSProvider>(mimoApiKey);
    }

    LOG_INFO(TEXT("TTS engine: Auto -> SAPI (no API keys available)"));
    return std::make_unique<SapiTTSProvider>();
}