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

    if (ttsEngine == "minimax") {
        if (!minimaxApiKey.empty()) {
            LOG_INFO(TEXT("TTS engine set to MiniMax (explicit)"));
            return std::make_unique<MiniMaxTTSProvider>(minimaxApiKey);
        }
        // Explicit minimax but no key - fall back to SAPI
        LOG_WARNING(TEXT("MiniMax TTS API key is empty, falling back to SAPI"));
        return std::make_unique<SapiTTSProvider>();
    }

    if (ttsEngine == "manbo") {
        LOG_INFO(TEXT("TTS engine set to Manbo (explicit)"));
        return std::make_unique<ManboTTSProvider>();
    }

    if (ttsEngine == "mimo") {
        if (mimoApiKey.empty()) {
            LOG_WARNING(TEXT("MiMo TTS API key is empty, falling back to SAPI"));
            return std::make_unique<SapiTTSProvider>();
        }
        LOG_INFO(TEXT("TTS engine set to MiMo (explicit)"));
        return std::make_unique<XiaomiTTSProvider>(mimoApiKey);
    }

    // AUTO mode: manbo -> minimax -> mimo -> sapi
    // Manbo does not require API key, always available
    LOG_INFO(TEXT("TTS engine: Auto -> Manbo"));
    return std::make_unique<ManboTTSProvider>();
}