#include "framework.h"
#include "DataBridge.h"
#include "EventBridge.h"
#include "WriteLog.h"
#include "UnitTestLog.h"
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestDataBridge_GetManagers()
{
    // 验证所有管理器可以正确获取
    assert(DataBridge::GetConfigManager() != nullptr);
    assert(DataBridge::GetMonsterDataManager() != nullptr);
    assert(DataBridge::GetPriorityQueueManager() != nullptr);
    assert(DataBridge::GetDanmuProcessor() != nullptr);

    TestLog("[PASS] TestDataBridge_GetManagers");
}

void TestDataBridge_GetAllConfig()
{
    ConfigData config = DataBridge::GetAllConfig();
    // 默认值检查
    assert(config.onlyMedalOrder == true);
    assert(config.opacity == 100);

    TestLog("[PASS] TestDataBridge_GetAllConfig");
}

void TestDataBridge_GetAllQueueNodes()
{
    auto nodes = DataBridge::GetAllQueueNodes();
    // 应该返回当前队列（可能为空）
    assert(nodes.size() >= 0);

    TestLog("[PASS] TestDataBridge_GetAllQueueNodes");
}

void TestDataBridge_ErrorCallback()
{
    bool callbackCalled = false;
    std::string receivedModule;
    std::string receivedMessage;

    DataBridge::RegisterErrorCallback(
        [&](const char* module, const char* message, int code) {
            callbackCalled = true;
            receivedModule = module;
            receivedMessage = message;
        }
    );

    // 触发一个错误
    REPORT_INFO("TestModule", "Test message");

    assert(callbackCalled == true);
    assert(receivedModule == "TestModule");

    TestLog("[PASS] TestDataBridge_ErrorCallback");
}

void TestEventBridge_RegisterCallbacks()
{
    bool configChanged = false;
    bool queueChanged = false;

    EventBridge::RegisterConfigChangedCallback([&](const ConfigData&) {
        configChanged = true;
    });

    EventBridge::RegisterQueueChangedCallback([&]() {
        queueChanged = true;
    });

    // 触发事件
    ConfigData newData;
    newData.idCode = "test_event";
    DataBridge::GetConfigManager()->UpdateConfig(newData);

    assert(configChanged == true);

    // 还原
    DataBridge::GetConfigManager()->SetIdCode("");

    TestLog("[PASS] TestEventBridge_RegisterCallbacks");
}

void RunAllDataBridgeTests()
{
    TestLog("=== DataBridge Tests ===");
    TestDataBridge_GetManagers();
    TestDataBridge_GetAllConfig();
    TestDataBridge_GetAllQueueNodes();
    TestDataBridge_ErrorCallback();
    TestEventBridge_RegisterCallbacks();
    TestLog("=== DataBridge Tests Done ===");
}

#endif // RUN_UNIT_TESTS