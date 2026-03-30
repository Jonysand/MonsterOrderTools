#pragma once
#include "framework.h"
#include "ConfigManager.h"
#include "PriorityQueueManager.h"
#include "DanmuProcessor.h"
#include <functional>

// C++/CLI 事件桥接接口
// 提供C++事件到C#事件的转换机制
// C#层通过回调函数注册，C++层在事件发生时调用回调

class EventBridge
{
public:
    // 通用回调函数类型
    using VoidCallback = std::function<void()>;
    using ConfigChangedCallback = std::function<void(const ConfigData&)>;
    using DanmuProcessedCallback = std::function<void(const DanmuProcessResult&)>;

    // 注册配置变更回调
    static void RegisterConfigChangedCallback(const ConfigChangedCallback& callback)
    {
        ConfigManager::Inst()->AddConfigChangedListener(callback);
    }

    // 注册队列变更回调
    static void RegisterQueueChangedCallback(const VoidCallback& callback)
    {
        PriorityQueueManager::Inst()->AddQueueChangedListener(callback);
    }

    // 注册弹幕处理完成回调
    static void RegisterDanmuProcessedCallback(const DanmuProcessedCallback& callback)
    {
        DanmuProcessor::Inst()->AddDanmuProcessedListener(callback);
    }
};
