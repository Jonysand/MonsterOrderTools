#include "AIChatProvider.h"
#include "DeepSeekAIChatProvider.h"
#include <iostream>
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestDeepSeekAIChatProvider_Interface()
{
    static_assert(std::is_base_of<IAIChatProvider, DeepSeekAIChatProvider>::value,
        "DeepSeekAIChatProvider must inherit IAIChatProvider");
    std::cout << "[PASS] TestDeepSeekAIChatProvider_Interface" << std::endl;
}

void TestDeepSeekAIChatProvider_ProviderName()
{
    auto provider = std::make_unique<DeepSeekAIChatProvider>("test_key");
    assert(provider->GetProviderName() == "deepseek");
    std::cout << "[PASS] TestDeepSeekAIChatProvider_ProviderName" << std::endl;
}

void TestDeepSeekAIChatProvider_InitialState()
{
    // IsAvailable() 依据 apiKey 是否为空判断：空 key 不可用
    auto provider = std::make_unique<DeepSeekAIChatProvider>("");
    assert(provider->IsAvailable() == false);
    std::cout << "[PASS] TestDeepSeekAIChatProvider_InitialState" << std::endl;
}

// 回归测试：请求体需按 DeepSeek 官方文档开启思考模式（thinking=low 思考强度）
void TestDeepSeekAIChatProvider_ThinkingModeRequestBody()
{
    std::string body = DeepSeekAIChatProvider::BuildRequestBody("test prompt");
    auto j = json::parse(body);

    assert(j["model"] == "deepseek-v4-flash");
    assert(j.contains("thinking"));
    assert(j["thinking"]["type"] == "enabled");
    assert(j["reasoning_effort"] == "low");
    assert(j["messages"].is_array() && j["messages"].size() == 1);
    assert(j["messages"][0]["role"] == "user");
    assert(j["messages"][0]["content"] == "test prompt");

    std::cout << "[PASS] TestDeepSeekAIChatProvider_ThinkingModeRequestBody" << std::endl;
}

void TestDeepSeekAIChatProviderFactory_Create()
{
    auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"deepseek","chat_api_key":"test_key"})");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "deepseek");
    std::cout << "[PASS] TestDeepSeekAIChatProviderFactory_Create" << std::endl;
}

void TestDeepSeekAIChatProviderFactory_EmptyApiKey()
{
    auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"deepseek","chat_api_key":""})");
    assert(provider == nullptr);
    std::cout << "[PASS] TestDeepSeekAIChatProviderFactory_EmptyApiKey" << std::endl;
}

void TestDeepSeekAIChatProviderFactory_UnsupportedProvider()
{
    auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"unknown","chat_api_key":"test_key"})");
    assert(provider == nullptr);
    std::cout << "[PASS] TestDeepSeekAIChatProviderFactory_UnsupportedProvider" << std::endl;
}

void TestDeepSeekAIChatProviderFactory_InvalidJson()
{
    auto provider = AIChatProviderFactory::Create("invalid json");
    assert(provider == nullptr);
    std::cout << "[PASS] TestDeepSeekAIChatProviderFactory_InvalidJson" << std::endl;
}

void RunDeepSeekAIChatProviderTests()
{
    std::cout << "========== DeepSeek AIChatProvider Tests ==========" << std::endl;
    TestDeepSeekAIChatProvider_Interface();
    TestDeepSeekAIChatProvider_ProviderName();
    TestDeepSeekAIChatProvider_InitialState();
    TestDeepSeekAIChatProvider_ThinkingModeRequestBody();
    TestDeepSeekAIChatProviderFactory_Create();
    TestDeepSeekAIChatProviderFactory_EmptyApiKey();
    TestDeepSeekAIChatProviderFactory_UnsupportedProvider();
    TestDeepSeekAIChatProviderFactory_InvalidJson();
    std::cout << "========== DeepSeek AIChatProvider Tests: ALL PASS ==========" << std::endl;
}

#endif