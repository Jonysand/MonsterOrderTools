#pragma once
#include "framework.h"
#include "DataBridge.h"
#include <msclr/marshal_cppstd.h>

namespace MonsterOrderBridge
{
    // 配置数据托管包装类
    public ref class ConfigProxy
    {
    public:
        ConfigProxy()
        {
            Refresh();
        }

        void Refresh()
        {
            auto config = DataBridge::GetAllConfig();
            IdCode = gcnew System::String(config.idCode.c_str());
            OnlyMedalOrder = config.onlyMedalOrder;
            EnableVoice = config.enableVoice;
            SpeechRate = config.speechRate;
            SpeechPitch = config.speechPitch;
            SpeechVolume = config.speechVolume;
            OnlySpeekWearingMedal = config.onlySpeekWearingMedal;
            OnlySpeekGuardLevel = config.onlySpeekGuardLevel;
            OnlySpeekPaidGift = config.onlySpeekPaidGift;
            Opacity = config.opacity;
            TtsEngine = gcnew System::String(config.ttsEngine.c_str());
            MimoApiKey = gcnew System::String(config.mimoApiKey.c_str());
            MimoVoice = gcnew System::String(config.mimoVoice.c_str());
            MimoStyle = gcnew System::String(config.mimoStyle.c_str());
            MimoDialect = gcnew System::String(config.mimoDialect.c_str());
            MimoRole = gcnew System::String(config.mimoRole.c_str());
            MimoAudioFormat = gcnew System::String(config.mimoAudioFormat.c_str());
            MimoSpeed = config.mimoSpeed;
            TopPosX = config.topPosX;
            TopPosY = config.topPosY;
            DefaultMarqueeText = gcnew System::String(config.defaultMarqueeText.c_str());
            TtsCacheDaysToKeep = config.ttsCacheDaysToKeep;
            EnableCaptainCheckinAI = config.enableCaptainCheckinAI;
            CheckinTriggerWords = gcnew System::String(config.checkinTriggerWords.c_str());
        }

        void Apply()
        {
            ConfigData data;
            data.idCode = msclr::interop::marshal_as<std::string>(IdCode);
            data.onlyMedalOrder = OnlyMedalOrder;
            data.enableVoice = EnableVoice;
            data.speechRate = SpeechRate;
            data.speechPitch = SpeechPitch;
            data.speechVolume = SpeechVolume;
            data.onlySpeekWearingMedal = OnlySpeekWearingMedal;
            data.onlySpeekGuardLevel = OnlySpeekGuardLevel;
            data.onlySpeekPaidGift = OnlySpeekPaidGift;
            data.opacity = Opacity;
            data.ttsEngine = msclr::interop::marshal_as<std::string>(TtsEngine);
            data.mimoApiKey = msclr::interop::marshal_as<std::string>(MimoApiKey);
            data.mimoVoice = msclr::interop::marshal_as<std::string>(MimoVoice);
            data.mimoStyle = msclr::interop::marshal_as<std::string>(MimoStyle);
            data.mimoDialect = msclr::interop::marshal_as<std::string>(MimoDialect);
            data.mimoRole = msclr::interop::marshal_as<std::string>(MimoRole);
            data.mimoAudioFormat = msclr::interop::marshal_as<std::string>(MimoAudioFormat);
            data.mimoSpeed = MimoSpeed;
            data.topPosX = TopPosX;
            data.topPosY = TopPosY;
            data.defaultMarqueeText = msclr::interop::marshal_as<std::string>(DefaultMarqueeText);
            data.ttsCacheDaysToKeep = TtsCacheDaysToKeep;
            data.enableCaptainCheckinAI = EnableCaptainCheckinAI;
            data.checkinTriggerWords = msclr::interop::marshal_as<std::string>(CheckinTriggerWords);

            DataBridge::UpdateAllConfig(data);
        }

        // 属性
        property System::String^ IdCode;
        property bool OnlyMedalOrder;
        property bool EnableVoice;
        property int SpeechRate;
        property int SpeechPitch;
        property int SpeechVolume;
        property bool OnlySpeekWearingMedal;
        property int OnlySpeekGuardLevel;
        property bool OnlySpeekPaidGift;
        property int Opacity;
        property System::String^ TtsEngine;
        property System::String^ MimoApiKey;
        property System::String^ MimoVoice;
        property System::String^ MimoStyle;
        property System::String^ MimoDialect;
        property System::String^ MimoRole;
        property System::String^ MimoAudioFormat;
        property float MimoSpeed;
        property double TopPosX;
        property double TopPosY;
        property System::String^ DefaultMarqueeText;
        property int TtsCacheDaysToKeep;
        property bool EnableCaptainCheckinAI;
        property System::String^ CheckinTriggerWords;
    };

    // 队列节点托管包装类
    public ref class QueueNodeProxy
    {
    public:
        property System::String^ UserId;
        property long long TimeStamp;
        property bool Priority;
        property System::String^ UserName;
        property System::String^ MonsterName;
        property int GuardLevel;
        property int TemperedLevel;

        static QueueNodeProxy^ FromCppData(const QueueNodeData& data)
        {
            auto node = gcnew QueueNodeProxy();
            node->UserId = gcnew System::String(data.userId.c_str());
            node->TimeStamp = data.timeStamp;
            node->Priority = data.priority;
            node->UserName = gcnew System::String(data.userName.c_str());
            node->MonsterName = gcnew System::String(data.monsterName.c_str());
            node->GuardLevel = data.guardLevel;
            node->TemperedLevel = data.temperedLevel;
            return node;
        }
    };

    // 优先级队列托管包装类
    public ref class PriorityQueueProxy
    {
    public:
        PriorityQueueProxy()
        {
            Refresh();
        }

        void Refresh()
        {
            auto nodes = DataBridge::GetAllQueueNodes();
            Nodes->Clear();
            for (const auto& node : nodes)
            {
                Nodes->Add(QueueNodeProxy::FromCppData(node));
            }
        }

        property System::Collections::Generic::List<QueueNodeProxy^>^ Nodes;
    };

    // 怪物数据托管包装类
    public ref class MonsterDataProxy
    {
    public:
        MonsterDataProxy()
        {
            Nodes = gcnew System::Collections::Generic::List<MonsterInfoProxy^>();
            Refresh();
        }

        void Refresh()
        {
            auto mgr = DataBridge::GetMonsterDataManager();
            auto info = mgr->GetAllMonsterInfo();
            Nodes->Clear();
            for (const auto& monster : info)
            {
                auto proxy = gcnew MonsterInfoProxy();
                proxy->MonsterName = gcnew System::String(monster.name.c_str());
                proxy->IconUrl = gcnew System::String(monster.iconUrl.c_str());
                proxy->TemperedLevel = monster.defaultTemperedLevel;
                Nodes->Add(proxy);
            }
        }

        property System::Collections::Generic::List<MonsterInfoProxy^>^ Nodes;
    };

    public ref class MonsterInfoProxy
    {
    public:
        property System::String^ MonsterName;
        property System::String^ IconUrl;
        property int TemperedLevel;
    };

    // 数据桥接器单例 - C#层访问C++数据的统一入口
    public ref class DataBridgeManager
    {
    public:
        static DataBridgeManager^ Inst()
        {
            if (instance == nullptr)
            {
                instance = gcnew DataBridgeManager();
            }
            return instance;
        }

        bool Initialize()
        {
            return DataBridge::Initialize();
        }

        void Shutdown()
        {
            DataBridge::Shutdown();
        }

        // 获取配置代理
        property ConfigProxy^ Config
        {
            ConfigProxy^ get()
            {
                if (configProxy == nullptr)
                    configProxy = gcnew ConfigProxy();
                return configProxy;
            }
        }

        // 获取队列代理
        property PriorityQueueProxy^ Queue
        {
            PriorityQueueProxy^ get()
            {
                if (queueProxy == nullptr)
                    queueProxy = gcnew PriorityQueueProxy();
                return queueProxy;
            }
        }

        // 获取怪物数据代理
        property MonsterDataProxy^ MonsterData
        {
            MonsterDataProxy^ get()
            {
                if (monsterProxy == nullptr)
                    monsterProxy = gcnew MonsterDataProxy();
                return monsterProxy;
            }
        }

        // 刷新所有数据
        void RefreshAll()
        {
            if (configProxy) configProxy->Refresh();
            if (queueProxy) queueProxy->Refresh();
            if (monsterProxy) monsterProxy->Refresh();
        }

    private:
        DataBridgeManager() : Nodes(gcnew System::Collections::Generic::List<QueueNodeProxy^>()) {}
        static DataBridgeManager^ instance = nullptr;
        ConfigProxy^ configProxy = nullptr;
        PriorityQueueProxy^ queueProxy = nullptr;
        MonsterDataProxy^ monsterProxy = nullptr;
        System::Collections::Generic::List<QueueNodeProxy^>^ Nodes;
    };
}
