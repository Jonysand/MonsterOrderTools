#include "framework.h"
#include "DanmuProcessor.h"
#include "MonsterDataManager.h"
#include "PriorityQueueManager.h"
#include "UnitTestLog.h"
#include <cassert>
#include <filesystem>
#include <fstream>

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

static void EnsurePriorityTestMonsterData()
{
    MonsterDataManager* monsterMgr = MonsterDataManager::Inst();
    if (!monsterMgr->IsLoaded())
    {
        std::string testDir = "MonsterOrderWilds_configs";
        std::string testPath = testDir + "/monster_list_test.json";
        if (!std::filesystem::exists(testDir))
            std::filesystem::create_directories(testDir);
        if (!std::filesystem::exists(testPath))
        {
            std::ofstream file(testPath);
            file << R"({
                "火龙": {
                    "图标地址": "https://example.com/fire_dragon.png",
                    "别称": ["火龙"],
                    "默认历战等级": 0
                }
            })";
            file.close();
        }
        monsterMgr->LoadJsonData(testPath);
    }
}

void TestPriority_TwoStep_Basic()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    DanmuData order;
    order.userId = "priority_user_1";
    order.userName = "优先用户1";
    order.message = "点怪火龙";
    order.timestamp = 1000;
    order.guardLevel = 3;
    order.hasMedal = true;
    DanmuProcessResult orderResult = processor->ProcessDanmu(order);
    assert(orderResult.addedToQueue == true);

    DanmuData prior;
    prior.userId = "priority_user_1";
    prior.userName = "优先用户1";
    prior.message = "优先";
    prior.timestamp = 1001;
    prior.guardLevel = 3;
    prior.hasMedal = true;
    DanmuProcessResult priorResult = processor->ProcessDanmu(prior);
    assert(priorResult.priorityUpdated == true);
    assert(priorResult.userName == "优先用户1");
    assert(priorResult.monsterName == "火龙");
    assert(priorResult.addedToQueue == false);
    assert(priorResult.shouldSpeak == false);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == true);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_TwoStep_Basic");
}

void TestPriority_NonGuard_NoUpdate()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    QueueNodeData node;
    node.userId = "priority_user_noguard";
    node.userName = "普通用户";
    node.monsterName = "火龙";
    node.timeStamp = 1000;
    node.priority = false;
    node.guardLevel = 0;
    queueMgr->Enqueue(node);

    DanmuData prior;
    prior.userId = "priority_user_noguard";
    prior.userName = "普通用户";
    prior.message = "优先";
    prior.timestamp = 1001;
    prior.guardLevel = 0;
    prior.hasMedal = true;
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == false);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == false);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_NonGuard_NoUpdate");
}

void TestPriority_NotInQueue_NoUpdate()
{
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    DanmuData prior;
    prior.userId = "priority_user_absent";
    prior.userName = "路人";
    prior.message = "优先";
    prior.timestamp = 1001;
    prior.guardLevel = 3;
    prior.hasMedal = true;
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == false);
    assert(queueMgr->GetCount() == 0);

    TestLog("[PASS] TestPriority_NotInQueue_NoUpdate");
}

void TestPriority_NoMedalFilterExempt()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    queueMgr->Clear();

    processor->SetOnlyMedalOrder(false);
    DanmuData order;
    order.userId = "priority_user_nomedal";
    order.userName = "无牌优先用户";
    order.message = "点怪火龙";
    order.timestamp = 1000;
    order.guardLevel = 3;
    order.hasMedal = true;
    DanmuProcessResult orderResult = processor->ProcessDanmu(order);
    assert(orderResult.addedToQueue == true);

    processor->SetOnlyMedalOrder(true);
    DanmuData prior;
    prior.userId = "priority_user_nomedal";
    prior.userName = "无牌优先用户";
    prior.message = "优先";
    prior.timestamp = 1001;
    prior.guardLevel = 3;
    prior.hasMedal = false;
    DanmuProcessResult priorResult = processor->ProcessDanmu(prior);
    assert(priorResult.priorityUpdated == true);

    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();
    TestLog("[PASS] TestPriority_NoMedalFilterExempt");
}

void TestPriority_Idempotent()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    DanmuData order;
    order.userId = "priority_user_repeat";
    order.userName = "重复优先用户";
    order.message = "点怪火龙";
    order.timestamp = 1000;
    order.guardLevel = 3;
    order.hasMedal = true;
    DanmuProcessResult orderResult = processor->ProcessDanmu(order);
    assert(orderResult.addedToQueue == true);

    DanmuData prior;
    prior.userId = "priority_user_repeat";
    prior.userName = "重复优先用户";
    prior.message = "优先";
    prior.timestamp = 1001;
    prior.guardLevel = 3;
    prior.hasMedal = true;
    DanmuProcessResult first = processor->ProcessDanmu(prior);
    assert(first.priorityUpdated == true);

    DanmuProcessResult second = processor->ProcessDanmu(prior);
    assert(second.priorityUpdated == false);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == true);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_Idempotent");
}

static void EnqueuePriorityTestOrder(DanmuProcessor* processor, const std::string& userId, const std::string& userName)
{
    DanmuData order;
    order.userId = userId;
    order.userName = userName;
    order.message = "点怪火龙";
    order.timestamp = 1000;
    order.guardLevel = 3;
    order.hasMedal = true;
    DanmuProcessResult orderResult = processor->ProcessDanmu(order);
    assert(orderResult.addedToQueue == true);
}

static DanmuData MakePriorityDanmu(const std::string& userId, const std::string& userName, const std::string& message)
{
    DanmuData prior;
    prior.userId = userId;
    prior.userName = userName;
    prior.message = message;
    prior.timestamp = 1001;
    prior.guardLevel = 3;
    prior.hasMedal = true;
    return prior;
}

void TestPriority_SentenceContainsKeyword_NoUpdate()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    EnqueuePriorityTestOrder(processor, "priority_user_sentence", "句中优先用户");

    DanmuData prior = MakePriorityDanmu("priority_user_sentence", "句中优先用户", "这个怪优先打");
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == false);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == false);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_SentenceContainsKeyword_NoUpdate");
}

void TestPriority_ExactInsertQueueWord_Updates()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    EnqueuePriorityTestOrder(processor, "priority_user_exact", "插队用户");

    DanmuData prior = MakePriorityDanmu("priority_user_exact", "插队用户", "插队");
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == true);
    assert(result.monsterName == "火龙");

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == true);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_ExactInsertQueueWord_Updates");
}

void TestPriority_PaddedWithSpaces_Updates()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    EnqueuePriorityTestOrder(processor, "priority_user_padded", "空格优先用户");

    DanmuData prior = MakePriorityDanmu("priority_user_padded", "空格优先用户", " 优先 ");
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == true);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == true);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_PaddedWithSpaces_Updates");
}

void TestPriority_WithPunctuation_NoUpdate()
{
    EnsurePriorityTestMonsterData();
    DanmuProcessor* processor = DanmuProcessor::Inst();
    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    processor->SetOnlyMedalOrder(false);
    queueMgr->Clear();

    EnqueuePriorityTestOrder(processor, "priority_user_punct", "标点优先用户");

    DanmuData prior = MakePriorityDanmu("priority_user_punct", "标点优先用户", "优先！");
    DanmuProcessResult result = processor->ProcessDanmu(prior);
    assert(result.priorityUpdated == false);

    auto nodes = queueMgr->GetAllNodes();
    assert(nodes.size() == 1);
    assert(nodes[0].priority == false);

    queueMgr->Clear();
    TestLog("[PASS] TestPriority_WithPunctuation_NoUpdate");
}

// 运行所有测试
void RunAllDanmuProcessorTests()
{
    TestLog("=== DanmuProcessor Tests ===");
    TestParseDanmuJson_Basic();
    TestParseDanmuJson_NoGuardLevel();
    TestGenerateSpeakText_Basic();
    TestDanmuFilter_OnlyMedalOrder();
    TestPriority_TwoStep_Basic();
    TestPriority_NonGuard_NoUpdate();
    TestPriority_NotInQueue_NoUpdate();
    TestPriority_NoMedalFilterExempt();
    TestPriority_Idempotent();
    TestPriority_SentenceContainsKeyword_NoUpdate();
    TestPriority_ExactInsertQueueWord_Updates();
    TestPriority_PaddedWithSpaces_Updates();
    TestPriority_WithPunctuation_NoUpdate();
    TestLog("=== DanmuProcessor Tests Done ===");
}

#endif // RUN_UNIT_TESTS