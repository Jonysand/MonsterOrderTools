#include "ITTSProvider.h"
#include "TTSProvider.h"
#include <iostream>
#include <cassert>
#include <string>

#ifdef RUN_UNIT_TESTS

void TestITTSProvider_Interface()
{
    static_assert(sizeof(ITTSProvider) > 0, "ITTSProvider should be non-empty");
    std::cout << "[PASS] TestITTSProvider_Interface" << std::endl;
}

void TestTTSRequest_Fields()
{
    TTSRequest req;
    req.text = "test text";

    assert(req.text == "test text");

    std::cout << "[PASS] TestTTSRequest_Fields" << std::endl;
}

void TestTTSResponse_Fields()
{
    TTSResponse resp;
    resp.success = true;
    resp.audioData = {0x48, 0x49};
    resp.errorMsg = "";

    assert(resp.success == true);
    assert((int)resp.audioData.size() == 2);

    std::cout << "[PASS] TestTTSResponse_Fields" << std::endl;
}

void TestSapiTTSProvider_IsAvailable()
{
    SapiTTSProvider sapi;
    assert(sapi.GetProviderName() == "sapi");
    assert(sapi.IsAvailable() == true);
    assert(sapi.GetLastError().empty());
    std::cout << "[PASS] TestSapiTTSProvider_IsAvailable" << std::endl;
}

void TestSapiTTSProvider_RequestTTS_Callback()
{
    SapiTTSProvider sapi;
    TTSRequest req;
    req.text = "test";

    bool callbackInvoked = false;
    sapi.RequestTTS(req, [&callbackInvoked](const TTSResponse& resp) {
        callbackInvoked = true;
    });
    
    assert(callbackInvoked == true);
    std::cout << "[PASS] TestSapiTTSProvider_RequestTTS_Callback" << std::endl;
}

void TestXiaomiTTSProvider_BuildRequest()
{
    XiaomiTTSProvider xiaomi("test_api_key");
    assert(xiaomi.GetProviderName() == "xiaomi");
    assert(xiaomi.IsAvailable() == true);

    // Note: BuildRequestBody now reads from ConfigManager, not TTSRequest
    // This test is disabled until we have ConfigManager mocking
    std::cout << "[SKIP] TestXiaomiTTSProvider_BuildRequest - reads from ConfigManager" << std::endl;
}

void TestXiaomiTTSProvider_HashtagToStyle_MoveTagToFront()
{
    XiaomiTTSProvider xiaomi("test_key");
    
    // 测试标签在文本中间的情况：标签应该被移到最前面
    std::string input1 = "阿斗说：#高兴#你好";
    std::string result1 = xiaomi.HashtagToStyle(input1);
    assert(result1 == "<style>高兴</style>阿斗说：你好");
    
    // 测试标签已经在最前面的情况：保持不变
    std::string input2 = "#高兴#阿斗说：你好";
    std::string result2 = xiaomi.HashtagToStyle(input2);
    assert(result2 == "<style>高兴</style>阿斗说：你好");
    
    // 测试没有标签的情况：保持不变
    std::string input3 = "阿斗说：你好";
    std::string result3 = xiaomi.HashtagToStyle(input3);
    assert(result3 == "阿斗说：你好");
    
    // 测试多个标签的情况：只移动第一个标签
    std::string input4 = "你好#高兴#世界#悲伤#结束";
    std::string result4 = xiaomi.HashtagToStyle(input4);
    assert(result4 == "<style>高兴</style>你好世界<style>悲伤</style>结束");
    
    // 测试空字符串
    std::string input5 = "";
    std::string result5 = xiaomi.HashtagToStyle(input5);
    assert(result5 == "");
    
    std::cout << "[PASS] TestXiaomiTTSProvider_HashtagToStyle_MoveTagToFront" << std::endl;
}

void TestXiaomiTTSProvider_BuildRequestHeaders()
{
    XiaomiTTSProvider xiaomi("my_api_key");
    std::string headers = xiaomi.BuildRequestHeaders("my_api_key");
    
    assert(headers.find("Content-Type: application/json") != std::string::npos);
    assert(headers.find("Authorization: Bearer my_api_key") != std::string::npos);
    
    std::cout << "[PASS] TestXiaomiTTSProvider_BuildRequestHeaders" << std::endl;
}

void TestXiaomiTTSProvider_ParseResponse_Success()
{
    XiaomiTTSProvider xiaomi("test_key");
    // MiMo API 响应格式: choices[0].message.audio.data
    std::string responseBody = "{\"choices\":[{\"message\":{\"audio\":{\"data\":\"SEk=\"}}}]}";
    
    auto resp = xiaomi.ParseResponse(responseBody, 200);
    assert(resp.success == true);
    assert(resp.audioData.size() == 2);
    assert(resp.audioData[0] == 0x48);  // 'H'
    assert(resp.audioData[1] == 0x49);  // 'I'
    
    std::cout << "[PASS] TestXiaomiTTSProvider_ParseResponse_Success" << std::endl;
}

void TestXiaomiTTSProvider_ParseResponse_Error()
{
    XiaomiTTSProvider xiaomi("test_key");
    std::string responseBody = "{\"error\":{\"message\":\"invalid api key\"}}";
    
    auto resp = xiaomi.ParseResponse(responseBody, 401);
    assert(resp.success == false);
    assert(!resp.errorMsg.empty());
    
    std::cout << "[PASS] TestXiaomiTTSProvider_ParseResponse_Error" << std::endl;
}

void TestMiniMaxTTSProvider_BuildRequest()
{
    MiniMaxTTSProvider minimax("test_api_key");
    assert(minimax.GetProviderName() == "minimax");
    assert(minimax.IsAvailable() == true);

    // Note: BuildRequestBody now reads from ConfigManager, not TTSRequest
    // This test is disabled until we have ConfigManager mocking
    std::cout << "[SKIP] TestMiniMaxTTSProvider_BuildRequest - reads from ConfigManager" << std::endl;
}

void TestMiniMaxTTSProvider_BuildRequestHeaders()
{
    MiniMaxTTSProvider minimax("my_api_key");
    std::string headers = minimax.BuildRequestHeaders("my_api_key");
    
    assert(headers.find("Content-Type: application/json") != std::string::npos);
    assert(headers.find("Authorization: Bearer my_api_key") != std::string::npos);
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_BuildRequestHeaders" << std::endl;
}

void TestMiniMaxTTSProvider_ParseResponse_Success()
{
    MiniMaxTTSProvider minimax("test_key");
    std::string responseBody = "{\"data\":{\"audio\":\"4849\",\"status\":2}}";
    
    auto resp = minimax.ParseResponse(responseBody);
    assert(resp.success == true);
    assert(resp.audioData.size() == 2);
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_ParseResponse_Success" << std::endl;
}

void TestMiniMaxTTSProvider_HexToBytes()
{
    MiniMaxTTSProvider minimax("test_key");
    
    // "4849" hex = 'H'(0x48) 'I'(0x49)
    auto result = minimax.HexToBytes("4849");
    assert(result.size() == 2);
    assert(result[0] == 0x48);
    assert(result[1] == 0x49);
    
    // "616263" hex = 'a'(0x61) 'b'(0x62) 'c'(0x63)
    result = minimax.HexToBytes("616263");
    assert(result.size() == 3);
    assert(result[0] == 0x61);
    assert(result[1] == 0x62);
    assert(result[2] == 0x63);
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_HexToBytes" << std::endl;
}

void TestMiniMaxTTSProvider_ParseResponse_WithHexAudio()
{
    MiniMaxTTSProvider minimax("test_key");
    // "4849" is hex encoded "HI"
    std::string responseBody = "{\"data\":{\"audio\":\"4849\",\"status\":2}}";
    
    auto resp = minimax.ParseResponse(responseBody);
    assert(resp.success == true);
    // Hex "4849" = bytes [0x48, 0x49] = 'H', 'I'
    assert(resp.audioData.size() == 2);
    assert(resp.audioData[0] == 0x48);  // 'H'
    assert(resp.audioData[1] == 0x49);  // 'I'
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_ParseResponse_WithHexAudio" << std::endl;
}

void TestMiniMaxTTSProvider_ParseResponse_Error()
{
    MiniMaxTTSProvider minimax("test_key");
    std::string responseBody = "{\"error\":{\"message\":\"invalid api key\"}}";
    
    auto resp = minimax.ParseResponse(responseBody);
    assert(resp.success == false);
    assert(!resp.errorMsg.empty());
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_ParseResponse_Error" << std::endl;
}

void TestManboTTSProvider_BuildRequestUrl()
{
    ManboTTSProvider manbo;
    TTSRequest req;
    req.text = "你好世界";
    
    std::string url = manbo.BuildRequestUrl(req);
    assert(url.find("/api/speech/AiChat/") != std::string::npos);
    assert(url.find("module=audio") != std::string::npos);
    assert(url.find("voice=") != std::string::npos);
    
    std::cout << "[PASS] TestManboTTSProvider_BuildRequestUrl" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_Success()
{
    ManboTTSProvider manbo;
    std::string responseBody = "{\"code\":200,\"message\":\"生成音频成功\",\"data\":{\"audio_url\":\"https://example.com/audio.wav\"}}";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == true);
    assert(resp.errorMsg == "https://example.com/audio.wav");
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_Success" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_Error()
{
    ManboTTSProvider manbo;
    std::string responseBody = "{\"code\":500,\"message\":\"服务器错误\"}";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == false);
    assert(resp.errorMsg == "服务器错误");
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_Error" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_InvalidJson()
{
    ManboTTSProvider manbo;
    std::string responseBody = "not json at all";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == false);
    assert(!resp.errorMsg.empty());
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_InvalidJson" << std::endl;
}

void TestManboTTSProvider_IsAvailable()
{
    ManboTTSProvider manbo;
    assert(manbo.GetProviderName() == "manbo");
    assert(manbo.IsAvailable() == true);
    assert(manbo.GetLastError().empty());
    
    std::cout << "[PASS] TestManboTTSProvider_IsAvailable" << std::endl;
}

void TestTTSProviderFactory_Create_ManboExplicit()
{
    auto provider = TTSProviderFactory::Create(
        "mimo_key",
        "minimax_key",
        "manbo");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "manbo");
    std::cout << "[PASS] TestTTSProviderFactory_Create_ManboExplicit" << std::endl;
}

void TestTTSProviderFactory_Create_Auto_ManboFirst()
{
    auto provider = TTSProviderFactory::Create(
        "mimo_key",
        "minimax_key",
        "auto");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "manbo");
    std::cout << "[PASS] TestTTSProviderFactory_Create_Auto_ManboFirst" << std::endl;
}

void TestTTSProviderFactory_Create_Auto_ManboFallback()
{
    auto provider = TTSProviderFactory::Create(
        "",
        "",
        "auto");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "manbo");
    std::cout << "[PASS] TestTTSProviderFactory_Create_Auto_ManboFallback" << std::endl;
}

void TestTTSProviderFactory_Create_MiMoExplicit()
{
    auto provider = TTSProviderFactory::Create(
        "mimo_key",
        "minimax_key",
        "mimo");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "xiaomi");
    std::cout << "[PASS] TestTTSProviderFactory_Create_MiMoExplicit" << std::endl;
}

void TestTTSProviderFactory_Create_SapiExplicit()
{
    auto provider = TTSProviderFactory::Create(
        "mimo_key",
        "minimax_key",
        "sapi");
    assert(provider != nullptr);
    assert(provider->GetProviderName() == "sapi");
    std::cout << "[PASS] TestTTSProviderFactory_Create_SapiExplicit" << std::endl;
}

void RunTTSProviderTests()
{
    std::cout << "========== TTSProvider Tests ==========" << std::endl;
    TestITTSProvider_Interface();
    TestTTSRequest_Fields();
    TestTTSResponse_Fields();
    // TestSapiTTSProvider_IsAvailable();  // Requires TTSManager, disabled
    // TestSapiTTSProvider_RequestTTS_Callback();  // Requires TTSManager, disabled
    TestXiaomiTTSProvider_BuildRequest();
    TestXiaomiTTSProvider_HashtagToStyle_MoveTagToFront();
    TestXiaomiTTSProvider_BuildRequestHeaders();
    TestXiaomiTTSProvider_ParseResponse_Success();
    TestXiaomiTTSProvider_ParseResponse_Error();
    TestMiniMaxTTSProvider_BuildRequest();
    TestMiniMaxTTSProvider_BuildRequestHeaders();
    TestMiniMaxTTSProvider_ParseResponse_Success();
    TestMiniMaxTTSProvider_HexToBytes();
    TestMiniMaxTTSProvider_ParseResponse_WithHexAudio();
    TestMiniMaxTTSProvider_ParseResponse_Error();
    TestManboTTSProvider_BuildRequestUrl();
    TestManboTTSProvider_ParseApiResponse_Success();
    TestManboTTSProvider_ParseApiResponse_Error();
    TestManboTTSProvider_ParseApiResponse_InvalidJson();
    TestManboTTSProvider_IsAvailable();
    // TestTTSProviderFactory_Create_Sapi();  // Requires SapiTTSProvider, disabled
    TestTTSProviderFactory_Create_Auto_ManboFirst();
    TestTTSProviderFactory_Create_Auto_ManboFallback();
    TestTTSProviderFactory_Create_ManboExplicit();
    TestTTSProviderFactory_Create_MiMoExplicit();
    TestTTSProviderFactory_Create_SapiExplicit();
    std::cout << "========== TTSProvider Tests: ALL PASS ==========" << std::endl;
}

#endif
