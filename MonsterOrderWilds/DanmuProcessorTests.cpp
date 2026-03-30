#include "framework.h"
#include "DanmuProcessor.h"
#include "MonsterDataManager.h"
#include "PriorityQueueManager.h"
#include "UnitTestLog.h"
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestParseDanmuJson_Basic()
{
    std::string jsonStr = R"({
        "uid": 12345,
        "uname": "测试用户",
        "msg": "点怪火龙",
        "send_time": 1700000000,
        "guard_level": 3,
        "fans_medal_wearing_status": true
    })";

    DanmuProcessor* processor = DanmuProcessor::Inst();
    DanmuData data = processor->ParseDanmuJson(jsonStr);

    assert(data.userId == "12345");
    assert(data.userName == "测试用户");
    assert(data.message == "点怪火龙");
    assert(data.guardLevel == 3);
    assert(data.hasMedal == true);

    TestLog("[PASS] TestParseDanmuJson_Basic");
}

void TestParseDanmuJson_NoGuardLevel()
{
    std::string jsonStr = R"({
        "uid": 99999,
        "uname": "普通用户",
        "msg": "你好",
        "send_time": 1700000001
    })";

    DanmuProcessor* processor = DanmuProcessor::Inst();
    DanmuData data = processor->ParseDanmuJson(jsonStr);

    assert(data.userId == "99999");
    assert(data.userName == "普通用户");
    assert(data.guardLevel == 0);

    TestLog("[PASS] TestParseDanmuJson_NoGuardLevel");
}

void TestGenerateSpeakText_Basic()
{
    DanmuProcessor* processor = DanmuProcessor::Inst();
    std::string result = processor->GenerateSpeakText("用户A", "火龙");
    assert(result == "用户A点怪火龙");

    TestLog("[PASS] TestGenerateSpeakText_Basic");
}

void TestDanmuFilter_OnlyMedalOrder()
{
    DanmuProcessor* processor = DanmuProcessor::Inst();
    processor->SetOnlyMedalOrder(true);

    DanmuData data;
    data.hasMedal = false;
    data.message = "点怪火龙";

    DanmuProcessResult result = processor->ProcessDanmu(data);
    assert(result.matched == false);  // 没有粉丝牌，不匹配

    processor->SetOnlyMedalOrder(false);

    TestLog("[PASS] TestDanmuFilter_OnlyMedalOrder");
}

// 运行所有测试
void RunAllDanmuProcessorTests()
{
    TestLog("=== DanmuProcessor Tests ===");
    TestParseDanmuJson_Basic();
    TestParseDanmuJson_NoGuardLevel();
    TestGenerateSpeakText_Basic();
    TestDanmuFilter_OnlyMedalOrder();
    TestLog("=== DanmuProcessor Tests Done ===");
}

#endif // RUN_UNIT_TESTS