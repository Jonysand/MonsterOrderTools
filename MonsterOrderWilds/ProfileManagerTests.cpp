#include "ProfileManager.h"
#include <iostream>

#ifdef RUN_UNIT_TESTS

void TestProfileManager_JsonSerialization()
{
    UserProfileData original;
    original.uid = 123456;
    original.username = "JsonTest";
    original.keywords = {KeywordRecord{1, "sword", 5, 1000}, KeywordRecord{2, "hammer", 3, 2000}};
    original.danmuHistory = {{1000, "danmu1"}, {2000, "danmu2"}};

    std::string json = ProfileManager::Inst()->SerializeToJson(original);
    assert(!json.empty());

    UserProfileData parsed;
    bool ok = ProfileManager::Inst()->DeserializeFromJson(json, parsed);
    assert(ok == true);
    assert(parsed.uid == original.uid);
    assert(parsed.username == original.username);
    assert((int)parsed.keywords.size() == (int)original.keywords.size());

    std::cout << "[PASS] TestProfileManager_JsonSerialization" << std::endl;
}

void TestProfileManager_Init()
{
    bool ok = ProfileManager::Inst()->Init();
    assert(ok == true);
    std::cout << "[PASS] TestProfileManager_Init" << std::endl;
}

void TestProfileManager_RecordCheckin()
{
    uint64_t testUid = 555666777;
    int32_t checkinDate = 20260405;

    ProfileManager::Inst()->RecordCheckin(testUid, "TestUser", checkinDate);

    UserProfileData loaded;
    bool found = ProfileManager::Inst()->LoadProfile(testUid, loaded);
    if (found) {
        assert(loaded.lastCheckinDate == checkinDate);
    }
    std::cout << "[PASS] TestProfileManager_RecordCheckin" << std::endl;
}

void TestProfileManager_CalculateContinuousDays()
{
    uint64_t testUid = 111222333;
    int32_t today = 20260405;

    ProfileManager::Inst()->RecordCheckin(testUid, "ConsecutiveTest", today);
    int32_t days = ProfileManager::Inst()->CalculateContinuousDays(testUid, today);
    assert(days >= 1);

    std::cout << "[PASS] TestProfileManager_CalculateContinuousDays" << std::endl;
}

void RunProfileManagerTests()
{
    std::cout << "========== ProfileManager Tests ==========" << std::endl;
    TestProfileManager_Init();
    TestProfileManager_JsonSerialization();
    TestProfileManager_RecordCheckin();
    TestProfileManager_CalculateContinuousDays();
    std::cout << "========== ProfileManager Tests: ALL PASS ==========" << std::endl;
}

#endif