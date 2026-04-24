#include "framework.h"
#include "TTSProvider.h"
#include "WriteLog.h"

std::shared_ptr<ITTSProvider> TTSProviderFactory::Create(
    const std::string& mimoApiKey,
    const std::string& minimaxApiKey,
    const std::string& ttsEngine) {

    if (ttsEngine == "sapi") {
        LOG_INFO(TEXT("TTS engine set to SAPI"));
        return std::make_shared<SapiTTSProvider>();
    }

    if (ttsEngine == "mimo") {
        if (mimoApiKey.empty()) {
            LOG_WARNING(TEXT("MiMo TTS API key is empty, falling back to SAPI"));
            return std::make_shared<SapiTTSProvider>();
        }
        LOG_INFO(TEXT("TTS engine set to MiMo (explicit)"));
        return std::make_shared<XiaomiTTSProvider>(mimoApiKey);
    }

    if (ttsEngine == "minimax") {
        if (minimaxApiKey.empty()) {
            LOG_WARNING(TEXT("MiniMax TTS API key is empty, falling back to SAPI"));
            return std::make_shared<SapiTTSProvider>();
        }
        LOG_INFO(TEXT("TTS engine set to MiniMax (explicit)"));
        return std::make_shared<MiniMaxTTSProvider>(minimaxApiKey);
    }

    if (ttsEngine == "manbo") {
        LOG_INFO(TEXT("TTS engine set to Manbo (explicit)"));
        return std::make_shared<ManboTTSProvider>();
    }

    // AUTO mode: manbo -> minimax -> mimo -> sapi
    LOG_INFO(TEXT("TTS engine: Auto -> Manbo"));
    return std::make_shared<ManboTTSProvider>();
}