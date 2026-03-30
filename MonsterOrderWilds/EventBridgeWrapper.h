#pragma once
#include "framework.h"
#include "EventBridge.h"
#include <msclr/marshal_cppstd.h>

namespace MonsterOrderBridge
{
    // 事件桥接器 - C++事件到C#事件的转换
    // 使用回调函数而非事件，避免C++/CLI事件复杂性
    public ref class EventBridgeManager
    {
    public:
        static EventBridgeManager^ Inst()
        {
            if (instance == nullptr)
            {
                instance = gcnew EventBridgeManager();
            }
            return instance;
        }

        // 初始化事件桥接
        void Initialize()
        {
            // 配置变更回调
            EventBridge::RegisterConfigChangedCallback([this](const ConfigData&) {
                if (configChangedCallback != nullptr)
                {
                    configChangedCallback();
                }
            });

            // 队列变更回调
            EventBridge::RegisterQueueChangedCallback([this]() {
                if (queueChangedCallback != nullptr)
                {
                    queueChangedCallback();
                }
            });

            // 弹幕处理完成回调
            EventBridge::RegisterDanmuProcessedCallback([this](const DanmuProcessResult& result) {
                if (danmuProcessedCallback != nullptr)
                {
                    danmuProcessedCallback(
                        gcnew System::String(result.userName.c_str()),
                        gcnew System::String(result.monsterName.c_str())
                    );
                }
            });
        }

        // 设置配置变更回调
        void SetConfigChangedCallback(System::Action^ callback)
        {
            configChangedCallback = callback;
        }

        // 设置队列变更回调
        void SetQueueChangedCallback(System::Action^ callback)
        {
            queueChangedCallback = callback;
        }

        // 设置弹幕处理完成回调
        void SetDanmuProcessedCallback(System::Action<System::String^, System::String^>^ callback)
        {
            danmuProcessedCallback = callback;
        }

    private:
        EventBridgeManager() {}
        static EventBridgeManager^ instance = nullptr;

        System::Action^ configChangedCallback = nullptr;
        System::Action^ queueChangedCallback = nullptr;
        System::Action<System::String^, System::String^>^ danmuProcessedCallback = nullptr;
    };
}
