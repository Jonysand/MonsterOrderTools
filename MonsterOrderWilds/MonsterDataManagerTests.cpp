#include "framework.h"
#include "MonsterDataManager.h"
#include "UnitTestLog.h"
#include <cassert>
#include <fstream>
#include <filesystem>

#ifdef RUN_UNIT_TESTS

void TestMonsterDataManager_NotLoaded()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();
    // 未加载时，匹配返回空结果
    MonsterMatchResult result = mgr->GetMatchedMonsterName("点怪火龙");
    assert(result.HasMatch() == false);
    assert(result.monsterName.empty());

    TestLog("[PASS] TestMonsterDataManager_NotLoaded");
}

void TestMonsterDataManager_CreateTestJson()
{
    // 创建测试用的monster_list.json
    std::string testDir = "MonsterOrderWilds_configs";
    std::string testPath = testDir + "/monster_list_test.json";

    if (!std::filesystem::exists(testDir))
        std::filesystem::create_directories(testDir);

    std::ofstream file(testPath);
    file << R"({
        "火龙": {
            "图标地址": "https://example.com/fire_dragon.png",
            "别称": ["火龙", "炎龙"],
            "默认历战等级": 0
        },
        "冰咒龙": {
            "图标地址": "https://example.com/ice_dragon.png",
            "别称": ["冰龙", "冰咒"],
            "默认历战等级": 1
        }
    })";
    file.close();

    TestLog("[PASS] TestMonsterDataManager_CreateTestJson");
}

void TestMonsterDataManager_LoadTestData()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();
    bool loaded = mgr->LoadJsonData("MonsterOrderWilds_configs/monster_list_test.json");
    assert(loaded == true);
    assert(mgr->IsLoaded() == true);

    TestLog("[PASS] TestMonsterDataManager_LoadTestData");
}

void TestMonsterDataManager_MatchByName()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    MonsterMatchResult result = mgr->GetMatchedMonsterName("点怪火龙");
    assert(result.HasMatch() == true);
    assert(result.monsterName == "火龙");

    TestLog("[PASS] TestMonsterDataManager_MatchByName");
}

void TestMonsterDataManager_MatchByNickname()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    MonsterMatchResult result = mgr->GetMatchedMonsterName("我要打炎龙");
    assert(result.HasMatch() == true);
    assert(result.monsterName == "火龙");

    TestLog("[PASS] TestMonsterDataManager_MatchByNickname");
}

void TestMonsterDataManager_MatchTemperedLevel()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    MonsterMatchResult result = mgr->GetMatchedMonsterName("点怪冰龙");
    assert(result.HasMatch() == true);
    assert(result.monsterName == "冰咒龙");
    assert(result.temperedLevel == 1);

    TestLog("[PASS] TestMonsterDataManager_MatchTemperedLevel");
}

void TestMonsterDataManager_NoMatch()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    MonsterMatchResult result = mgr->GetMatchedMonsterName("你好世界");
    assert(result.HasMatch() == false);

    TestLog("[PASS] TestMonsterDataManager_NoMatch");
}

void TestMonsterDataManager_GetIconUrl()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    std::string url = mgr->GetMatchedMonsterIconUrl("火龙");
    assert(url == "https://example.com/fire_dragon.png");

    std::string emptyUrl = mgr->GetMatchedMonsterIconUrl("不存在的怪物");
    assert(emptyUrl.empty());

    TestLog("[PASS] TestMonsterDataManager_GetIconUrl");
}

void TestMonsterDataManager_GetAllNames()
{
    MonsterDataManager* mgr = MonsterDataManager::Inst();

    auto names = mgr->GetAllMonsterNames();
    assert(names.size() == 2);

    TestLog("[PASS] TestMonsterDataManager_GetAllNames");
}

void TestMonsterDataManager_CleanupTestFile()
{
    std::filesystem::remove("MonsterOrderWilds_configs/monster_list_test.json");
    TestLog("[PASS] TestMonsterDataManager_CleanupTestFile");
}

void RunAllMonsterDataManagerTests()
{
    TestLog("=== MonsterDataManager Tests ===");
    TestMonsterDataManager_NotLoaded();
    TestMonsterDataManager_CreateTestJson();
    TestMonsterDataManager_LoadTestData();
    TestMonsterDataManager_MatchByName();
    TestMonsterDataManager_MatchByNickname();
    TestMonsterDataManager_MatchTemperedLevel();
    TestMonsterDataManager_NoMatch();
    TestMonsterDataManager_GetIconUrl();
    TestMonsterDataManager_GetAllNames();
    TestMonsterDataManager_CleanupTestFile();
    TestLog("=== MonsterDataManager Tests Done ===");
}

#endif // RUN_UNIT_TESTS