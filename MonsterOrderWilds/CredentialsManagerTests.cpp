#ifdef RUN_UNIT_TESTS
#include "CredentialsManager.h"
#include "UnitTestLog.h"
#include <cassert>
#include <fstream>

void TestCredentialsFileExists() {
    std::string filepath = "MonsterOrderWilds_configs/credentials.dat";
    std::ifstream file(filepath, std::ios::binary);
    assert(file.good());
    TestLog("[PASS] TestCredentialsFileExists");
}

void TestLoadCredentials() {
    bool loaded = LoadCredentials();
    assert(loaded);
    TestLog("[PASS] TestLoadCredentials");
}

void TestGetAPPCredentials() {
    LoadCredentials();
    std::string appId = GetAPP_ID();
    std::string accessKeyId = GetACCESS_KEY_ID();
    std::string accessKeySecret = GetACCESS_KEY_SECRET();
    
    assert(!appId.empty());
    assert(!accessKeyId.empty());
    assert(!accessKeySecret.empty());
    
    TestLog("[PASS] TestGetAPPCredentials");
}

void TestGetMIMOApiKey() {
    LoadCredentials();
    std::string mimoApiKey = GetMIMO_API_KEY();
    assert(!mimoApiKey.empty());
    TestLog("[PASS] TestGetMIMOApiKey");
}

void TestAI_PROVIDER_GetCredentials() {
    LoadCredentials();
    std::string aiProvider = GetAI_PROVIDER();
    assert(!aiProvider.empty());
    TestLog("[PASS] TestAI_PROVIDER_GetCredentials");
}

void TestAI_PROVIDER_ParseJson() {
    LoadCredentials();
    std::string aiProvider = GetAI_PROVIDER();
    
    assert(aiProvider.find("chat_provider") != std::string::npos);
    assert(aiProvider.find("chat_api_key") != std::string::npos);
    assert(aiProvider.find("tts_provider") != std::string::npos);
    assert(aiProvider.find("tts_api_key") != std::string::npos);
    
    TestLog("[PASS] TestAI_PROVIDER_ParseJson");
}

void TestAI_PROVIDER_InvalidJson() {
    std::string invalidJson = "not valid json {{{";
    bool canParse = true;
    try {
        json::parse(invalidJson);
        canParse = false;
    } catch (const std::exception&) {
        canParse = true;
    }
    assert(canParse);
    TestLog("[PASS] TestAI_PROVIDER_InvalidJson");
}

void TestAI_PROVIDER_EmptyFields() {
    std::string jsonWithEmptyFields = R"({"chat_provider":"minimax","chat_api_key":"","tts_provider":"xiaomi","tts_api_key":""})";
    bool canParse = true;
    try {
        auto j = json::parse(jsonWithEmptyFields);
        assert(j["chat_api_key"].get<std::string>().empty());
        assert(j["tts_api_key"].get<std::string>().empty());
    } catch (const std::exception&) {
        canParse = false;
    }
    assert(canParse);
    TestLog("[PASS] TestAI_PROVIDER_EmptyFields");
}

void RunAllCredentialsManagerTests()
{
    TestLog("=== CredentialsManager Tests ===");
    TestCredentialsFileExists();
    TestLoadCredentials();
    TestGetAPPCredentials();
    TestGetMIMOApiKey();
    TestAI_PROVIDER_GetCredentials();
    TestAI_PROVIDER_ParseJson();
    TestAI_PROVIDER_InvalidJson();
    TestAI_PROVIDER_EmptyFields();
    TestLog("=== CredentialsManager Tests Done ===");
}

#endif
