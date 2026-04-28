#pragma once
#include "framework.h"
#include "ConfigManager.h"
#include "ConfigFieldRegistry.h"
#include "MonsterDataManager.h"
#include "PriorityQueueManager.h"
#include "DanmuProcessor.h"
#if !ONLY_ORDER_MONSTER
#include "ProfileManager.h"
#include "CaptainCheckInModule.h"
#endif
#include "ErrorHandler.h"
#include <functional>
#include <string>

// C++/CLI 数据桥接接口
// 提供C#层访问C++数据的统一接口
// 设计为纯C++接口，由C++/CLI包装层调用
class DataBridge
{
public:
    // 跨层错误回调（C++ → C#）
    // C#层注册此回调来接收C++层的错误信息
    using ErrorNotificationCallback = std::function<void(const char* module, const char* message, int code)>;

    // 注册错误通知回调
    static void RegisterErrorCallback(const ErrorNotificationCallback& callback)
    {
        errorCallback_ = callback;
        ErrorHandler::Instance().AddListener([](const ErrorInfo& info) {
            if (errorCallback_)
            {
                errorCallback_(info.moduleName.c_str(), info.message.c_str(), info.errorCode);
            }
        });
    }

    // 获取配置管理器
    static ConfigManager* GetConfigManager()
    {
        return ConfigManager::Inst();
    }

    // 获取怪物数据管理器
    static MonsterDataManager* GetMonsterDataManager()
    {
        return MonsterDataManager::Inst();
    }

    // 获取优先级队列管理器
    static PriorityQueueManager* GetPriorityQueueManager()
    {
        return PriorityQueueManager::Inst();
    }

    // 获取弹幕处理器
    static DanmuProcessor* GetDanmuProcessor()
    {
        return DanmuProcessor::Inst();
    }

    // 批量获取配置数据（一次跨层调用获取所有配置）
    static ConfigData GetAllConfig()
    {
        return GetConfigManager()->GetConfig();
    }

    // 批量获取队列数据（一次跨层调用获取所有队列）
    static std::vector<QueueNodeData> GetAllQueueNodes()
    {
        return GetPriorityQueueManager()->GetAllNodes();
    }

    // 批量更新配置
    static void UpdateAllConfig(const ConfigData& data)
    {
        GetConfigManager()->UpdateConfig(data);
    }

    // 安全初始化（带异常捕获和错误通知）
    static bool Initialize()
    {
        try
        {
            ConfigFieldRegistry::RegisterAll();

            bool configLoaded = GetConfigManager()->LoadConfig();
            bool monsterLoaded = GetMonsterDataManager()->LoadJsonData();
            GetPriorityQueueManager()->LoadList();

            // 同步过滤条件到弹幕处理器
            const auto& config = GetConfigManager()->GetConfig();
            GetDanmuProcessor()->SetOnlyMedalOrder(config.onlyMedalOrder);
            GetDanmuProcessor()->SetOnlySpeekWearingMedal(config.onlySpeekWearingMedal);
            GetDanmuProcessor()->SetOnlySpeekGuardLevel(config.onlySpeekGuardLevel);
            GetDanmuProcessor()->SetOnlySpeekPaidGift(config.onlySpeekPaidGift);

#if !ONLY_ORDER_MONSTER
            // 初始化舰长签到模块
            ProfileManager::Inst()->Init();
            GetDanmuProcessor()->Init();
            CaptainCheckInModule::Inst()->Init();
#endif

            if (!configLoaded)
                REPORT_WARNING("DataBridge", "Config file not found, using defaults");
            if (!monsterLoaded)
                REPORT_WARNING("DataBridge", "Monster list not found");

            return true;
        }
        catch (const std::exception& e)
        {
            REPORT_CRITICAL("DataBridge", std::string("Initialize failed: ") + e.what(), -1);
            return false;
        }
    }

    // 安全关闭（带异常捕获）
    static void Shutdown()
    {
        try
        {
            GetConfigManager()->SaveConfig(true);
            GetPriorityQueueManager()->SaveList();
            REPORT_INFO("DataBridge", "Shutdown complete");
        }
        catch (const std::exception& e)
        {
            REPORT_ERROR("DataBridge", std::string("Shutdown error: ") + e.what(), -1);
        }
    }

private:
    static inline ErrorNotificationCallback errorCallback_;
};
