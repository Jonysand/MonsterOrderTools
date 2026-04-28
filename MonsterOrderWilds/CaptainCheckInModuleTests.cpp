#include "CaptainCheckInModule.h"
#include <iostream>
#include <cassert>
#include <sstream>
#include <thread>
#include <chrono>

#ifdef RUN_UNIT_TESTS

void TestCaptainCheckInModule_SetEnabled()
{
    CaptainCheckInModule::Inst()->SetEnabled(true);
    assert(CaptainCheckInModule::Inst()->IsEnabled() == true);
    CaptainCheckInModule::Inst()->SetEnabled(false);
    assert(CaptainCheckInModule::Inst()->IsEnabled() == false);
    CaptainCheckInModule::Inst()->SetEnabled(true);
    std::cout << "[PASS] TestCaptainCheckInModule_SetEnabled" << std::endl;
}

void TestCaptainCheckInModule_IsCheckinMessage()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("点怪") == false);
    std::cout << "[PASS] TestCaptainCheckInModule_IsCheckinMessage" << std::endl;
}

void TestCaptainCheckInModule_IsCheckinMessage_Custom()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("报名,报道");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报名") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报道") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    std::cout << "[PASS] TestCaptainCheckInModule_IsCheckinMessage_Custom" << std::endl;
}

void TestCaptainCheckInModule_PushDanmuEvent()
{
    CaptainDanmuEvent evt;
    evt.uid = "12345";
    evt.username = "test_user";
    evt.guardLevel = 3;
    evt.content = "test content";
    evt.serverTimestamp = 0;

    CaptainCheckInModule::Inst()->SetEnabled(true);
    CaptainCheckInModule::Inst()->PushDanmuEvent(evt);
    std::cout << "[PASS] TestCaptainCheckInModule_PushDanmuEvent" << std::endl;
}

void TestCaptainCheckInModule_GetUserProfile()
{
    UserProfile profile;
    bool found = CaptainCheckInModule::Inst()->GetUserProfile(99999, profile);
    assert(!found);
    std::cout << "[PASS] TestCaptainCheckInModule_GetUserProfile" << std::endl;
}

void TestCaptainCheckInModule_GetTopKeywords()
{
    std::vector<std::string> keywords = CaptainCheckInModule::Inst()->GetUserTopKeywords("12345");
    assert(keywords.size() == 0);
    std::cout << "[PASS] TestCaptainCheckInModule_GetTopKeywords" << std::endl;
}

void TestGetFallbackAnswer_FirstDay()
{
    CheckinEvent evt;
    evt.uid = "12345";
    evt.username = "舰长A";
    evt.continuousDays = 1;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长A打卡成功！");
    std::cout << "[PASS] TestGetFallbackAnswer_FirstDay" << std::endl;
}

void TestGetFallbackAnswer_ConsecutiveDays()
{
    CheckinEvent evt;
    evt.uid = "12345";
    evt.username = "舰长B";
    evt.continuousDays = 7;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长B连续第7天打卡！");
    std::cout << "[PASS] TestGetFallbackAnswer_ConsecutiveDays" << std::endl;
}

void TestGetFallbackAnswer_ManyDays()
{
    CheckinEvent evt;
    evt.uid = "12345";
    evt.username = "舰长C";
    evt.continuousDays = 100;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长C连续第100天打卡！");
    std::cout << "[PASS] TestGetFallbackAnswer_ManyDays" << std::endl;
}

void TestGetFallbackAnswer_ZeroDays()
{
    CheckinEvent evt;
    evt.uid = "12345";
    evt.username = "舰长D";
    evt.continuousDays = 0;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长D打卡成功！");
    std::cout << "[PASS] TestGetFallbackAnswer_ZeroDays" << std::endl;
}

void TestIsCheckinMessage_EmptyTriggerWords()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == false);
    std::cout << "[PASS] TestIsCheckinMessage_EmptyTriggerWords" << std::endl;
}

void TestIsCheckinMessage_SingleWord()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == false);
    std::cout << "[PASS] TestIsCheckinMessage_SingleWord" << std::endl;
}

void TestIsCheckinMessage_WithSpaces()
{
    CaptainCheckInModule::Inst()->SetTriggerWords(" 打卡 , 签到 ");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    std::cout << "[PASS] TestIsCheckinMessage_WithSpaces" << std::endl;
}

void TestIsCheckinMessage_ExactMatch()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到");
    // Only exact match should trigger
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    // Partial match should NOT trigger
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("我来打卡了") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡签到") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("今天来打卡") == false);
    std::cout << "[PASS] TestIsCheckinMessage_ExactMatch" << std::endl;
}

void TestIsCheckinMessage_CaseInsensitive()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("DaKa") == false);  // Not exact match
    std::cout << "[PASS] TestIsCheckinMessage_CaseInsensitive" << std::endl;
}

void TestSetTriggerWords_UpdatesCorrectly()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    
    CaptainCheckInModule::Inst()->SetTriggerWords("报名,报道");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报名") == true);
    std::cout << "[PASS] TestSetTriggerWords_UpdatesCorrectly" << std::endl;
}

void TestIsCheckinMessage_ChineseComma()
{
    // 测试中文逗号分隔（U+FF0C），这是默认配置格式
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡，签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("点怪") == false);
    
    // 测试中英文逗号混合
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到，报名");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报名") == true);
    
    // 测试中文逗号周围有空格时也能正确trim
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡 ， 签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    
    std::cout << "[PASS] TestIsCheckinMessage_ChineseComma" << std::endl;
}

void TestCalculateContinuousDays_ProfileManagerIntegration()
{
    std::string testUid = "888777666";
    int32_t today = 20260406;

    ProfileManager::Inst()->RecordCheckin(testUid, "TestUser", today);
    int32_t days = ProfileManager::Inst()->CalculateContinuousDays(testUid, today);

    assert(days >= 1);
    std::cout << "[PASS] TestCalculateContinuousDays_ProfileManagerIntegration" << std::endl;
}

void TestCheckinUpsertBehavior()
{
    std::string testUid = "999888777";
    int32_t date1 = 20260410;
    int32_t date2 = 20260411;

    ProfileManager::Inst()->RecordCheckin(testUid, "UpsertTestUser", date1);
    int32_t days1 = ProfileManager::Inst()->CalculateContinuousDays(testUid, date1);
    assert(days1 == 1);

    ProfileManager::Inst()->RecordCheckin(testUid, "UpsertTestUser", date1);
    int32_t days1Repeat = ProfileManager::Inst()->CalculateContinuousDays(testUid, date1);
    assert(days1Repeat == 1);

    ProfileManager::Inst()->RecordCheckin(testUid, "UpsertTestUser", date2);
    int32_t days2 = ProfileManager::Inst()->CalculateContinuousDays(testUid, date2);
    assert(days2 == 2);

    std::cout << "[PASS] TestCheckinUpsertBehavior" << std::endl;
}

void TestGenerateCheckinAnswerAsync_Callback()
{
    CheckinEvent evt;
    evt.uid = "123456";
    evt.username = "test_user";
    evt.continuousDays = 1;
    evt.checkinDate = 20260412;
    
    bool callbackCalled = false;
    CaptainCheckInModule::Inst()->GenerateCheckinAnswerAsync(evt, [&callbackCalled](const AnswerResult& result) {
        callbackCalled = true;
        assert(result.success);
        std::cout << "[ASYNC TEST] Callback received result: " << (result.isAiGenerated ? "AI" : "Fallback") 
                  << ", content: " << result.answerContent << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    assert(callbackCalled);
    std::cout << "[PASS] TestGenerateCheckinAnswerAsync_Callback" << std::endl;
}

void TestExtractKeywords_SkipsSingleHashtagLabel()
{
    CaptainCheckInModule::Inst()->Init();
    CaptainCheckInModule::Inst()->SetEnabled(true);

    std::string testUid = "999999001";

    CaptainDanmuEvent evt;
    evt.uid = testUid;
    evt.username = "test_user";
    evt.guardLevel = 3;
    evt.content = "今天#唱歌#很开心";
    evt.serverTimestamp = 0;
    evt.sendDate = 20260418;

    CaptainCheckInModule::Inst()->PushDanmuEvent(evt);

    UserProfile profile;
    bool found = CaptainCheckInModule::Inst()->GetUserProfile(testUid, profile);
    assert(found);
    assert(!profile.keywords.empty()); // 确保 jieba 正常工作且提取了关键词

    for (const auto& kw : profile.keywords) {
        assert(kw.word != "唱歌"); // hashtag 标签词不应被学习
    }

    std::cout << "[PASS] TestExtractKeywords_SkipsSingleHashtagLabel" << std::endl;
}

void TestExtractKeywords_SkipsMultipleHashtagLabels()
{
    CaptainCheckInModule::Inst()->Init();
    CaptainCheckInModule::Inst()->SetEnabled(true);

    std::string testUid = "999999002";

    CaptainDanmuEvent evt;
    evt.uid = testUid;
    evt.username = "test_user2";
    evt.guardLevel = 3;
    evt.content = "#高兴#地去#打卡#";
    evt.serverTimestamp = 0;
    evt.sendDate = 20260418;

    CaptainCheckInModule::Inst()->PushDanmuEvent(evt);

    UserProfile profile;
    bool found = CaptainCheckInModule::Inst()->GetUserProfile(testUid, profile);
    assert(found);
    assert(!profile.keywords.empty()); // 确保 jieba 正常工作且提取了关键词

    for (const auto& kw : profile.keywords) {
        assert(kw.word != "高兴"); // hashtag 标签词不应被学习
        assert(kw.word != "打卡"); // hashtag 标签词不应被学习
    }

    std::cout << "[PASS] TestExtractKeywords_SkipsMultipleHashtagLabels" << std::endl;
}

void RunCaptainCheckInModuleTests()
{
    std::cout << "========== CaptainCheckInModule Tests ==========" << std::endl;
    TestCaptainCheckInModule_SetEnabled();
    TestCaptainCheckInModule_IsCheckinMessage();
    TestCaptainCheckInModule_IsCheckinMessage_Custom();
    TestCaptainCheckInModule_PushDanmuEvent();
    TestCaptainCheckInModule_GetUserProfile();
    TestCaptainCheckInModule_GetTopKeywords();
    TestGetFallbackAnswer_FirstDay();
    TestGetFallbackAnswer_ConsecutiveDays();
    TestGetFallbackAnswer_ManyDays();
    TestGetFallbackAnswer_ZeroDays();
    TestIsCheckinMessage_EmptyTriggerWords();
    TestIsCheckinMessage_SingleWord();
    TestIsCheckinMessage_WithSpaces();
    TestIsCheckinMessage_ExactMatch();
    TestIsCheckinMessage_CaseInsensitive();
    TestSetTriggerWords_UpdatesCorrectly();
    TestIsCheckinMessage_ChineseComma();
    TestCalculateContinuousDays_ProfileManagerIntegration();
    TestCheckinUpsertBehavior();
    TestGenerateCheckinAnswerAsync_Callback();
    TestExtractKeywords_SkipsSingleHashtagLabel();
    TestExtractKeywords_SkipsMultipleHashtagLabels();
    std::cout << "========== CaptainCheckInModule Tests: ALL PASS ==========" << std::endl;
}

#endif