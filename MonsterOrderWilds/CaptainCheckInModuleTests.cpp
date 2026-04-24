#include "CaptainCheckInModule.h"
#include <iostream>
#include <cassert>
#include <sstream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdio>

#ifdef RUN_UNIT_TESTS

extern "C" void TestLog(const char* msg);

void TestCaptainCheckInModule_SetEnabled()
{
    CaptainCheckInModule::Inst()->SetEnabled(true);
    assert(CaptainCheckInModule::Inst()->IsEnabled() == true);
    CaptainCheckInModule::Inst()->SetEnabled(false);
    assert(CaptainCheckInModule::Inst()->IsEnabled() == false);
    CaptainCheckInModule::Inst()->SetEnabled(true);
    TestLog("[PASS] TestCaptainCheckInModule_SetEnabled");
}

void TestCaptainCheckInModule_IsCheckinMessage()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("点怪") == false);
    TestLog("[PASS] TestCaptainCheckInModule_IsCheckinMessage");
}

void TestCaptainCheckInModule_IsCheckinMessage_Custom()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("报名,报道");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报名") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报道") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    TestLog("[PASS] TestCaptainCheckInModule_IsCheckinMessage_Custom");
}

void TestCaptainCheckInModule_PushDanmuEvent()
{
    CaptainDanmuEvent evt;
    evt.uid = 12345;
    evt.username = "test_user";
    evt.guardLevel = 3;
    evt.content = "test content";
    evt.serverTimestamp = 0;

    CaptainCheckInModule::Inst()->SetEnabled(true);
    CaptainCheckInModule::Inst()->PushDanmuEvent(evt);
    TestLog("[PASS] TestCaptainCheckInModule_PushDanmuEvent");
}

void TestCaptainCheckInModule_GetUserProfile()
{
    const UserProfile* profile = CaptainCheckInModule::Inst()->GetUserProfile("99999");
    assert(profile == nullptr);
    TestLog("[PASS] TestCaptainCheckInModule_GetUserProfile");
}

void TestCaptainCheckInModule_GetTopKeywords()
{
    std::vector<std::string> keywords = CaptainCheckInModule::Inst()->GetUserTopKeywords("12345");
    assert(keywords.size() == 0);
    TestLog("[PASS] TestCaptainCheckInModule_GetTopKeywords");
}

void TestGetFallbackAnswer_FirstDay()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长A";
    evt.continuousDays = 1;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长A打卡成功！");
    TestLog("[PASS] TestGetFallbackAnswer_FirstDay");
}

void TestGetFallbackAnswer_ConsecutiveDays()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长B";
    evt.continuousDays = 7;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长B连续第7天打卡！");
    TestLog("[PASS] TestGetFallbackAnswer_ConsecutiveDays");
}

void TestGetFallbackAnswer_ManyDays()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长C";
    evt.continuousDays = 100;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长C连续第100天打卡！");
    TestLog("[PASS] TestGetFallbackAnswer_ManyDays");
}

void TestGetFallbackAnswer_ZeroDays()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长D";
    evt.continuousDays = 0;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长D打卡成功！累计0天");
    TestLog("[PASS] TestGetFallbackAnswer_ZeroDays");
}

void TestGetFallbackAnswer_WithCumulativeDays()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长E";
    evt.continuousDays = 5;
    evt.cumulativeDays = 20;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长E连续第5天打卡！累计20天");
    TestLog("[PASS] TestGetFallbackAnswer_WithCumulativeDays");
}

void TestGetFallbackAnswer_FirstDayWithCumulative()
{
    CheckinEvent evt;
    evt.uid = 12345;
    evt.username = "舰长F";
    evt.continuousDays = 1;
    evt.cumulativeDays = 1;
    evt.checkinDate = 20260406;
    
    std::string answer = CaptainCheckInModule::Inst()->GetFallbackAnswer(evt);
    assert(answer == "舰长F打卡成功！累计1天");
    TestLog("[PASS] TestGetFallbackAnswer_FirstDayWithCumulative");
}

void TestIsCheckinMessage_EmptyTriggerWords()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == false);
    TestLog("[PASS] TestIsCheckinMessage_EmptyTriggerWords");
}

void TestIsCheckinMessage_SingleWord()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == false);
    TestLog("[PASS] TestIsCheckinMessage_SingleWord");
}

void TestIsCheckinMessage_WithSpaces()
{
    CaptainCheckInModule::Inst()->SetTriggerWords(" 打卡 , 签到 ");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("签到") == true);
    TestLog("[PASS] TestIsCheckinMessage_WithSpaces");
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
    TestLog("[PASS] TestIsCheckinMessage_ExactMatch");
}

void TestIsCheckinMessage_CaseInsensitive()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("DaKa") == false);  // Not exact match
    TestLog("[PASS] TestIsCheckinMessage_CaseInsensitive");
}

void TestSetTriggerWords_UpdatesCorrectly()
{
    CaptainCheckInModule::Inst()->SetTriggerWords("打卡,签到");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == true);
    
    CaptainCheckInModule::Inst()->SetTriggerWords("报名,报道");
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("打卡") == false);
    assert(CaptainCheckInModule::Inst()->IsCheckinMessage("报名") == true);
    TestLog("[PASS] TestSetTriggerWords_UpdatesCorrectly");
}

void TestCalculateContinuousDays_ProfileManagerIntegration()
{
    std::string testUid = "888777666";
    int32_t today = 20260406;

    ProfileManager::Inst()->RecordCheckin(testUid, "TestUser", today);
    int32_t days = ProfileManager::Inst()->CalculateContinuousDays(testUid, today);

    assert(days >= 1);
    TestLog("[PASS] TestCalculateContinuousDays_ProfileManagerIntegration");
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

    TestLog("[PASS] TestCheckinUpsertBehavior");
}

void TestGenerateCheckinAnswerAsync_Callback()
{
    CheckinEvent evt;
    evt.uid = 123456;
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
    TestLog("[PASS] TestGenerateCheckinAnswerAsync_Callback");
}

// ===== StopWord Dictionary Tests =====

void TestLoadStopWords_Success()
{
    std::string tempFile = "test_stopwords_temp.utf8";
    {
        std::ofstream ofs(tempFile);
        ofs << "老白\n";
        ofs << "测试词\n";
        ofs << "\n";  // 空行，应被跳过
        ofs << "  带空格  \n";
    }

    CaptainCheckInModule::Inst()->stopWords_.clear();
    CaptainCheckInModule::Inst()->LoadStopWords(tempFile);

    assert(CaptainCheckInModule::Inst()->stopWords_.size() == 3);
    assert(CaptainCheckInModule::Inst()->stopWords_.find("老白") != CaptainCheckInModule::Inst()->stopWords_.end());
    assert(CaptainCheckInModule::Inst()->stopWords_.find("测试词") != CaptainCheckInModule::Inst()->stopWords_.end());
    assert(CaptainCheckInModule::Inst()->stopWords_.find("带空格") != CaptainCheckInModule::Inst()->stopWords_.end());

    std::remove(tempFile.c_str());
    TestLog("[PASS] TestLoadStopWords_Success");
}

void TestLoadStopWords_FileNotFound()
{
    CaptainCheckInModule::Inst()->stopWords_.clear();
    CaptainCheckInModule::Inst()->LoadStopWords("nonexistent_file.utf8");

    assert(CaptainCheckInModule::Inst()->stopWords_.empty());
    TestLog("[PASS] TestLoadStopWords_FileNotFound");
}

void TestIsStopWord_LoadedDict()
{
    std::string tempFile = "test_stopwords_lao.utf8";
    {
        std::ofstream ofs(tempFile);
        ofs << "老白\n";
        ofs << "的\n";
    }

    CaptainCheckInModule::Inst()->stopWords_.clear();
    CaptainCheckInModule::Inst()->LoadStopWords(tempFile);

    assert(CaptainCheckInModule::Inst()->IsStopWord("老白") == true);
    assert(CaptainCheckInModule::Inst()->IsStopWord("的") == true);

    std::remove(tempFile.c_str());
    TestLog("[PASS] TestIsStopWord_LoadedDict");
}

void TestIsStopWord_HardcodedFallback()
{
    CaptainCheckInModule::Inst()->stopWords_.clear();

    assert(CaptainCheckInModule::Inst()->IsStopWord("的") == true);
    assert(CaptainCheckInModule::Inst()->IsStopWord("了") == true);
    assert(CaptainCheckInModule::Inst()->IsStopWord("在") == true);

    TestLog("[PASS] TestIsStopWord_HardcodedFallback");
}

void TestIsStopWord_NotStopWord()
{
    CaptainCheckInModule::Inst()->stopWords_.clear();

    assert(CaptainCheckInModule::Inst()->IsStopWord("好吃") == false);
    assert(CaptainCheckInModule::Inst()->IsStopWord("游戏") == false);
    assert(CaptainCheckInModule::Inst()->IsStopWord("舰长") == false);

    TestLog("[PASS] TestIsStopWord_NotStopWord");
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
    TestGetFallbackAnswer_WithCumulativeDays();
    TestGetFallbackAnswer_FirstDayWithCumulative();
    TestIsCheckinMessage_EmptyTriggerWords();
    TestIsCheckinMessage_SingleWord();
    TestIsCheckinMessage_WithSpaces();
    TestIsCheckinMessage_ExactMatch();
    TestIsCheckinMessage_CaseInsensitive();
    TestSetTriggerWords_UpdatesCorrectly();
    TestCalculateContinuousDays_ProfileManagerIntegration();
    TestCheckinUpsertBehavior();
    TestLoadStopWords_Success();
    TestLoadStopWords_FileNotFound();
    TestIsStopWord_LoadedDict();
    TestIsStopWord_HardcodedFallback();
    TestIsStopWord_NotStopWord();
    TestGenerateCheckinAnswerAsync_Callback();
    std::cout << "========== CaptainCheckInModule Tests: ALL PASS ==========" << std::endl;
}

#endif