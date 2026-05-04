#include "framework.h"
#include "AIChatProvider.h"
#include "MiniMaxAIChatProvider.h"
#include "DeepSeekAIChatProvider.h"
#include "CredentialsManager.h"
#include "WriteLog.h"

std::unique_ptr<IAIChatProvider> AIChatProviderFactory::Create(
    const std::string& credentialJson) {
    try {
        auto cred = nlohmann::json::parse(credentialJson);

        std::string provider = cred.value("chat_provider", "minimax");
        std::string apiKey = cred.value("chat_api_key", "");

        if (apiKey.empty()) {
            LOG_ERROR(TEXT("AI Chat API key is empty"));
            return nullptr;
        }

        if (provider == "minimax") {
            return std::make_unique<MiniMaxAIChatProvider>(apiKey);
        }

        if (provider == "deepseek") {
            return std::make_unique<DeepSeekAIChatProvider>(apiKey);
        }

        LOG_ERROR(TEXT("Unsupported chat provider: %hs"), provider.c_str());
        return nullptr;
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("Failed to create AI chat provider: %hs"), e.what());
        return nullptr;
    }
}