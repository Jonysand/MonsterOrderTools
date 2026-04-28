#include "framework.h"
#include "ConfigManager.h"
#include "DanmuProcessor.h"
#include "UnitTestLog.h"
#include <cassert>
#include <filesystem>

#ifdef RUN_UNIT_TESTS

void TestConfigManager_DefaultValues()
{
    ConfigManager* mgr = ConfigManager::Inst();
    const auto& config = mgr->GetConfig();

    assert(config.onlyMedalOrder == true);
    assert(config.enableVoice == false);
    assert(config.speechRate == 0);
    assert(config.speechPitch == 0);
    assert(config.speechVolume == 100);
    assert(config.opacity == 100);
    assert(config.ttsEngine == "auto");

    TestLog("[PASS] TestConfigManager_DefaultValues");
}

void TestConfigManager_SetIdCode()
{
    ConfigManager* mgr = ConfigManager::Inst();
    mgr->SetIdCode("test_code_123");
    assert(mgr->GetConfig().idCode == "test_code_123");

    // 还原
    mgr->SetIdCode("");

    TestLog("[PASS] TestConfigManager_SetIdCode");
}

void TestConfigManager_SetFlags()
{
    ConfigManager* mgr = ConfigManager::Inst();

    mgr->SetEnableVoice(true);
    assert(mgr->GetConfig().enableVoice == true);

    mgr->SetOnlyMedalOrder(false);
    assert(mgr->GetConfig().onlyMedalOrder == false);

    mgr->SetOnlySpeekWearingMedal(true);
    assert(mgr->GetConfig().onlySpeekWearingMedal == true);

    mgr->SetOnlySpeekPaidGift(true);
    assert(mgr->GetConfig().onlySpeekPaidGift == true);

    // 还原
    mgr->SetEnableVoice(false);
    mgr->SetOnlyMedalOrder(true);
    mgr->SetOnlySpeekWearingMedal(false);
    mgr->SetOnlySpeekPaidGift(false);

    TestLog("[PASS] TestConfigManager_SetFlags");
}

void TestConfigManager_SetNumericValues()
{
    ConfigManager* mgr = ConfigManager::Inst();

    mgr->SetSpeechRate(5);
    assert(mgr->GetConfig().speechRate == 5);

    mgr->SetSpeechPitch(-3);
    assert(mgr->GetConfig().speechPitch == -3);

    mgr->SetSpeechVolume(80);
    assert(mgr->GetConfig().speechVolume == 80);

    mgr->SetOpacity(50);
    assert(mgr->GetConfig().opacity == 50);

    mgr->SetOnlySpeekGuardLevel(3);
    assert(mgr->GetConfig().onlySpeekGuardLevel == 3);

    // 还原
    mgr->SetSpeechRate(0);
    mgr->SetSpeechPitch(0);
    mgr->SetSpeechVolume(100);
    mgr->SetOpacity(100);
    mgr->SetOnlySpeekGuardLevel(0);

    TestLog("[PASS] TestConfigManager_SetNumericValues");
}

void TestConfigManager_SetTtsConfig()
{
    ConfigManager* mgr = ConfigManager::Inst();

    mgr->SetTtsEngine("mimo");
    assert(mgr->GetConfig().ttsEngine == "mimo");

    mgr->SetMimoVoice("default_zh");
    assert(mgr->GetConfig().mimoVoice == "default_zh");

    mgr->SetMimoStyle("温柔轻声");
    assert(mgr->GetConfig().mimoStyle == "温柔轻声");

    // 还原
    mgr->SetTtsEngine("auto");
    mgr->SetMimoVoice("mimo_default");
    mgr->SetMimoStyle("");

    TestLog("[PASS] TestConfigManager_SetTtsConfig");
}

void TestConfigManager_WindowPosition()
{
    ConfigManager* mgr = ConfigManager::Inst();

    mgr->SetWindowPosition(100.5, 200.3);
    assert(mgr->GetConfig().topPosX == 100.5);
    assert(mgr->GetConfig().topPosY == 200.3);

    // 还原
    mgr->SetWindowPosition(0.0, 0.0);

    TestLog("[PASS] TestConfigManager_WindowPosition");
}

void TestConfigManager_SaveAndLoad()
{
    ConfigManager* mgr = ConfigManager::Inst();

    // 设置测试值
    mgr->SetIdCode("save_test");
    mgr->SetSpeechVolume(75);
    bool saved = mgr->SaveConfig(true);
    assert(saved == true);

    // 修改值
    mgr->SetIdCode("changed");
    mgr->SetSpeechVolume(100);

    // 重新加载
    bool loaded = mgr->LoadConfig();
    assert(loaded == true);
    assert(mgr->GetConfig().idCode == "save_test");
    assert(mgr->GetConfig().speechVolume == 75);

    // 还原
    mgr->SetIdCode("");
    mgr->SetSpeechVolume(100);
    mgr->SaveConfig(true);

    TestLog("[PASS] TestConfigManager_SaveAndLoad");
}

void TestConfigManager_EventNotification()
{
    ConfigManager* mgr = ConfigManager::Inst();
    bool notified = false;

    mgr->AddConfigChangedListener([&notified](const ConfigData&) {
        notified = true;
    });

    mgr->SetIdCode("event_test");
    assert(notified == true);

    // 还原
    mgr->SetIdCode("");
    notified = false;

    TestLog("[PASS] TestConfigManager_EventNotification");
}

void TestConfigManager_OnlyMedalOrderSyncToDanmuProcessor()
{
    ConfigManager* mgr = ConfigManager::Inst();
    DanmuProcessor* processor = DanmuProcessor::Inst();

    processor->SetOnlyMedalOrder(true);
    assert(processor->GetOnlyMedalOrder() == true);

    mgr->SetOnlyMedalOrder(false);
    assert(processor->GetOnlyMedalOrder() == false);

    mgr->SetOnlyMedalOrder(true);
    assert(processor->GetOnlyMedalOrder() == true);

    TestLog("[PASS] TestConfigManager_OnlyMedalOrderSyncToDanmuProcessor");
}

void RunAllConfigManagerTests()
{
    TestLog("=== ConfigManager Tests ===");
    TestConfigManager_DefaultValues();
    TestConfigManager_SetIdCode();
    TestConfigManager_SetFlags();
    TestConfigManager_SetNumericValues();
    TestConfigManager_SetTtsConfig();
    TestConfigManager_WindowPosition();
    TestConfigManager_SaveAndLoad();
    TestConfigManager_EventNotification();
    TestConfigManager_OnlyMedalOrderSyncToDanmuProcessor();
    TestLog("=== ConfigManager Tests Done ===");
}

#endif // RUN_UNIT_TESTS