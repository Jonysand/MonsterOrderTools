#include "MimoTTSClient.h"
#include <iostream>
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestBuildRequestBody_Basic()
{
    MimoTTSClient::TTSRequest request;
    request.text = "Hello World";
    request.voice = "mimo_default";
    request.model = "mimo-v2-tts";
    request.responseFormat = "mp3";
    request.speed = 1.0f;
    request.style = "";

    MimoTTSClient client;
    std::string body = client.BuildRequestBody(request);
    
    assert(body.find("\"model\":\"mimo-v2-tts\"") != std::string::npos);
    assert(body.find("Hello World") != std::string::npos);
    assert(body.find("mimo_default") != std::string::npos);
    
    std::cout << "[PASS] TestBuildRequestBody_Basic" << std::endl;
}

void TestBuildRequestBody_WithStyle()
{
    MimoTTSClient::TTSRequest request;
    request.text = "test text";
    request.voice = "mimo_default";
    request.model = "mimo-v2-tts";
    request.responseFormat = "mp3";
    request.speed = 1.0f;
    request.style = "excited";

    MimoTTSClient client;
    std::string body = client.BuildRequestBody(request);
    
    assert(body.find("<style>excited</style>test text") != std::string::npos);
    
    std::cout << "[PASS] TestBuildRequestBody_WithStyle" << std::endl;
}

void TestBuildRequestHeaders()
{
    MimoTTSClient client;
    std::string headers = client.BuildRequestHeaders("test_api_key");
    
    assert(headers.find("Content-Type: application/json") != std::string::npos);
    assert(headers.find("Authorization: Bearer test_api_key") != std::string::npos);
    
    std::cout << "[PASS] TestBuildRequestHeaders" << std::endl;
}

void TestBuildRequestHeaders_Empty()
{
    MimoTTSClient client;
    std::string headers = client.BuildRequestHeaders("");
    
    assert(headers.find("Authorization: Bearer ") != std::string::npos);
    
    std::cout << "[PASS] TestBuildRequestHeaders_Empty" << std::endl;
}

void TestParseResponse_Success()
{
    MimoTTSClient client;
    std::string responseBody = R"({
        "audio": {
            "data": "SGVsbG8gV29ybGQ="
        }
    })";
    
    auto response = client.ParseResponse(responseBody, 200);
    
    assert(response.success == true);
    assert(!response.audioData.empty());
    
    std::cout << "[PASS] TestParseResponse_Success" << std::endl;
}

void TestParseResponse_Error()
{
    MimoTTSClient client;
    std::string responseBody = R"({
        "error": "Invalid API key"
    })";
    
    auto response = client.ParseResponse(responseBody, 401);
    
    assert(response.success == false);
    assert(response.errorMessage.find("Invalid API key") != std::string::npos);
    
    std::cout << "[PASS] TestParseResponse_Error" << std::endl;
}

void TestParseResponse_Empty()
{
    MimoTTSClient client;
    std::string responseBody = "";
    
    auto response = client.ParseResponse(responseBody, 200);
    
    assert(response.success == false);
    
    std::cout << "[PASS] TestParseResponse_Empty" << std::endl;
}

int main()
{
    std::cout << "Running MimoTTSClient Unit Tests..." << std::endl;
    
    TestBuildRequestBody_Basic();
    TestBuildRequestBody_WithStyle();
    TestBuildRequestHeaders();
    TestBuildRequestHeaders_Empty();
    TestParseResponse_Success();
    TestParseResponse_Error();
    TestParseResponse_Empty();
    
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

#endif
