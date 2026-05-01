#include "framework.h"
#include <Windows.h>
#include <cstdio>
#include <string>

#ifdef RUN_UNIT_TESTS

// Global test logger
static HANDLE g_LogFile = INVALID_HANDLE_VALUE;
static std::string g_LogFilePath;

void InitTestLogger() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    size_t pos = exePath.find_last_of("\\");
    if (pos != std::string::npos) {
        exePath = exePath.substr(0, pos);
    }
    g_LogFilePath = exePath + "\\test_results.txt";
    
    g_LogFile = CreateFileA(g_LogFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void CloseTestLogger() {
    if (g_LogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_LogFile);
        g_LogFile = INVALID_HANDLE_VALUE;
    }
}

// Export for test files
extern "C" void TestLog(const char* msg) {
    if (g_LogFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(g_LogFile, msg, (DWORD)strlen(msg), &written, NULL);
        WriteFile(g_LogFile, "\r\n", 2, &written, NULL);
    }
}

extern void RunAllConfigManagerTests();
extern void RunAllConfigManagerRegistryTests();
extern void RunAllMonsterDataManagerTests();
extern void RunAllPriorityQueueManagerTests();
extern void RunAllDanmuProcessorTests();
extern void RunAllStringProcessorTests();
extern void RunAllDataBridgeTests();
extern void RunAllCredentialsManagerTests();
extern void TestTTSCacheManager();
extern void TestDynamicComboConstants();
extern void RunProfileManagerTests();
extern void RunAIChatProviderTests();
extern void RunTTSProviderTests();
extern void RunCaptainCheckInModuleTests();
extern void RunRetroactiveCheckInModuleTests();
extern void RunDeepSeekAIChatProviderTests();
extern void RunLocalVoiceManagerTests();

int RunTests()
{
    InitTestLogger();
    
    TestLog("========================================");
    TestLog("  MonsterOrderWilds Unit Tests");
    TestLog("========================================");
    TestLog("");
    
    TestLog("Running ConfigManager tests...");
    RunAllConfigManagerTests();
    TestLog("");

    TestLog("Running ConfigManager Registry tests...");
    RunAllConfigManagerRegistryTests();
    TestLog("");
    
    TestLog("Running MonsterDataManager tests...");
    RunAllMonsterDataManagerTests();
    TestLog("");
    
    TestLog("Running PriorityQueueManager tests...");
    RunAllPriorityQueueManagerTests();
    TestLog("");
    
    TestLog("Running DanmuProcessor tests...");
    RunAllDanmuProcessorTests();
    TestLog("");
    
    TestLog("Running StringProcessor tests...");
    RunAllStringProcessorTests();
    TestLog("");
    
    TestLog("Running DataBridge tests...");
    RunAllDataBridgeTests();
    TestLog("");
    
    TestLog("Running CredentialsManager tests...");
    RunAllCredentialsManagerTests();
    TestLog("");
    
    TestLog("Running TTSCacheManager tests...");
    TestTTSCacheManager();
    TestLog("");
    
    TestLog("Running TextToSpeech tests...");
    TestDynamicComboConstants();
    TestLog("");

    TestLog("Running ProfileManager tests...");
    RunProfileManagerTests();
    TestLog("");

    TestLog("Running AIChatProvider tests...");
    RunAIChatProviderTests();
    TestLog("");

    TestLog("Running DeepSeek AIChatProvider tests...");
    RunDeepSeekAIChatProviderTests();
    TestLog("");

    TestLog("Running TTSProvider tests...");
    RunTTSProviderTests();
    TestLog("");

    TestLog("Running CaptainCheckInModule tests...");
    RunCaptainCheckInModuleTests();
    TestLog("");

    TestLog("Running RetroactiveCheckInModule tests...");
    RunRetroactiveCheckInModuleTests();
    TestLog("");

    TestLog("Running LocalVoiceManager tests...");
    RunLocalVoiceManagerTests();
    TestLog("");

    TestLog("========================================");
    TestLog("  All tests completed!");
    TestLog("========================================");
    
    OutputDebugStringA("Log saved to: ");
    OutputDebugStringA(g_LogFilePath.c_str());
    OutputDebugStringA("\n");
    
    CloseTestLogger();
    return 0;
}

int main(int argc, char* argv[])
{
    return RunTests();
}

#endif