#include "AIChatProvider.h"
#include <iostream>
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestAIChatProvider_Interface()
{
    static_assert(sizeof(IAIChatProvider) > 0, "IAIChatProvider should be non-empty");
    std::cout << "[PASS] TestAIChatProvider_Interface" << std::endl;
}

void TestAIChatProviderFactory_Create()
{
    auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"minimax","chat_api_key":"test_key"})");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "minimax");
    std::cout << "[PASS] TestAIChatProviderFactory_Create" << std::endl;
}

void TestAIChatProviderFactory_InvalidJson()
{
    auto provider = AIChatProviderFactory::Create("invalid json");
    assert(provider == nullptr);
    std::cout << "[PASS] TestAIChatProviderFactory_InvalidJson" << std::endl;
}

void TestAIChatProviderFactory_EmptyApiKey()
{
    auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"minimax","chat_api_key":""})");
    assert(provider == nullptr);
    std::cout << "[PASS] TestAIChatProviderFactory_EmptyApiKey" << std::endl;
}

void RunAIChatProviderTests()
{
    std::cout << "========== AIChatProvider Tests ==========" << std::endl;
    TestAIChatProvider_Interface();
    TestAIChatProviderFactory_Create();
    TestAIChatProviderFactory_InvalidJson();
    TestAIChatProviderFactory_EmptyApiKey();
    std::cout << "========== AIChatProvider Tests: ALL PASS ==========" << std::endl;
}

#endif