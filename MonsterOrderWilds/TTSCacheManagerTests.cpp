#ifdef RUN_UNIT_TESTS
#include "TTSCacheManager.h"
#include "ConfigManager.h"
#include "UnitTestLog.h"
#include "StringUtils.h"
#include <cassert>
#include <filesystem>

void TestTTSCacheManager() {
    ConfigManager::Inst()->LoadConfig();
    TTSCacheManager::Inst()->Initialize();
    
    std::wstring todayDir = TTSCacheManager::Inst()->GetTodayCacheDir();
    assert(!todayDir.empty());
    TestLog("[PASS] GetTodayCacheDir");
    
    std::wstring wtestText = L"user1 \x8BF4\xFF1Ahello";
    std::string testText = wstring_to_utf8(wtestText);
    std::string prefix = TTSCacheManager::Inst()->GetContentPrefix(testText);
    assert(prefix == "user1_hello");
    TestLog("[PASS] GetContentPrefix with Chinese separator");
    
    testText = "thank you gift";
    prefix = TTSCacheManager::Inst()->GetContentPrefix(testText);
    assert(prefix == "thank");
    TestLog("[PASS] GetContentPrefix without speaker");
    
    std::string shortText = "hi";
    prefix = TTSCacheManager::Inst()->GetContentPrefix(shortText);
    assert(prefix == "hi");
    TestLog("[PASS] GetContentPrefix short text");
    
    TestLog("[PASS] All TTSCacheManager tests passed!");
}
#endif