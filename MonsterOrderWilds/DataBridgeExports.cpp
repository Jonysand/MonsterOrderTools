#include "DataBridgeExports.h"
#include "ConfigFieldRegistry.h"
#include "WriteLog.h"
#include <cstring>
#include <mutex>

#if !ONLY_ORDER_MONSTER
#include "CaptainCheckInModule.h"
#include "RetroactiveCheckInModule.h"
#include "ProfileManager.h"
#endif

extern "C" {

    __declspec(dllexport) bool __stdcall DataBridge_Initialize()
    {
        try
        {
            return DataBridge::Initialize();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DataBridge_Initialize failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall DataBridge_Shutdown()
    {
        try
        {
            DataBridge::Shutdown();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DataBridge_Shutdown failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_SetValue(const char* key, const char* value, int type)
    {
        try
        {
            auto configMgr = DataBridge::GetConfigManager();

            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta)
            {
                LOG_ERROR(TEXT("Config_SetValue: unknown key=%hs"), key);
                return;
            }

            switch (type)
            {
            case CONFIG_TYPE_STRING:
                {
                    int wlen = MultiByteToWideChar(CP_ACP, 0, value, -1, nullptr, 0);
                    if (wlen > 0) {
                        std::wstring wstr(wlen - 1, L'\0');
                        MultiByteToWideChar(CP_ACP, 0, value, -1, &wstr[0], wlen);
                        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
                        if (utf8Len > 0) {
                            std::string utf8Str(utf8Len - 1, '\0');
                            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], utf8Len, nullptr, nullptr);
                            configMgr->SetValueByMeta(meta, &utf8Str);
                        }
                    } else {
                        std::string strVal(value);
                        configMgr->SetValueByMeta(meta, &strVal);
                    }
                }
                break;
            case CONFIG_TYPE_BOOL:
                {
                    bool boolVal = (_stricmp(value, "true") == 0 || strcmp(value, "1") == 0);
                    configMgr->SetValueByMeta(meta, &boolVal);
                }
                break;
            case CONFIG_TYPE_INT:
                {
                    int intVal = atoi(value);
                    configMgr->SetValueByMeta(meta, &intVal);
                }
                break;
            case CONFIG_TYPE_FLOAT:
                {
                    float floatVal = (float)atof(value);
                    configMgr->SetValueByMeta(meta, &floatVal);
                }
                break;
            case CONFIG_TYPE_DOUBLE:
                {
                    double doubleVal = atof(value);
                    configMgr->SetValueByMeta(meta, &doubleVal);
                }
                break;
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_SetValue failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_Save()
    {
        try
        {
            DataBridge::GetConfigManager()->SaveConfig(true);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_Save failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_GetString(const char* key, char* outValue, int bufferSize)
    {
        try
        {
            const auto& config = DataBridge::GetConfigManager()->GetConfig();
            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta)
            {
                if (outValue && bufferSize > 0) outValue[0] = '\0';
                return;
            }
            const char* value = ConfigFieldRegistry::GetString(meta, config);

            int wlen = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
            if (wlen > 0) {
                std::wstring wstr(wlen - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, value, -1, &wstr[0], wlen);
                int ansiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (ansiLen > 0) {
                    if (ansiLen <= bufferSize) {
                        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, outValue, bufferSize, nullptr, nullptr);
                    } else {
                        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, outValue, bufferSize, nullptr, nullptr);
                        if (bufferSize > 0) outValue[bufferSize - 1] = '\0';
                    }
                } else {
                    strncpy_s(outValue, bufferSize, value, bufferSize - 1);
                    if (bufferSize > 0) outValue[bufferSize - 1] = '\0';
                }
            } else {
                strncpy_s(outValue, bufferSize, value, bufferSize - 1);
                if (bufferSize > 0) outValue[bufferSize - 1] = '\0';
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_GetString failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_GetBool(const char* key, bool* outValue)
    {
        try
        {
            const auto& config = DataBridge::GetConfigManager()->GetConfig();
            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta) return;
            *outValue = ConfigFieldRegistry::GetBool(meta, config);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_GetBool failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_GetInt(const char* key, int* outValue)
    {
        try
        {
            const auto& config = DataBridge::GetConfigManager()->GetConfig();
            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta) return;
            *outValue = ConfigFieldRegistry::GetInt(meta, config);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_GetInt failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_GetFloat(const char* key, float* outValue)
    {
        try
        {
            const auto& config = DataBridge::GetConfigManager()->GetConfig();
            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta) return;
            *outValue = ConfigFieldRegistry::GetFloat(meta, config);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_GetFloat failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) void __stdcall Config_GetDouble(const char* key, double* outValue)
    {
        try
        {
            const auto& config = DataBridge::GetConfigManager()->GetConfig();
            auto* meta = ConfigFieldRegistry::Find(key);
            if (!meta) return;
            *outValue = ConfigFieldRegistry::GetDouble(meta, config);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("Config_GetDouble failed: key=%s, error=%s"), key, e.what());
        }
    }

    __declspec(dllexport) bool __stdcall DataBridge_IsMonsterDataLoaded()
    {
        try
        {
            return DataBridge::GetMonsterDataManager()->IsLoaded();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DataBridge_IsMonsterDataLoaded failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) bool __stdcall DataBridge_MatchMonsterName(const wchar_t* inputText, wchar_t* outMonsterName, int nameBufferSize, int* outTemperedLevel)
    {
        try
        {
            std::string narrowInput = WstringToUtf8(inputText);
            auto result = DataBridge::GetMonsterDataManager()->GetMatchedMonsterName(narrowInput);
            if (result.HasMatch())
            {
                if (outMonsterName && nameBufferSize > 0)
                {
                    std::wstring wideName = Utf8ToWstring(result.monsterName);
                    wcsncpy_s(outMonsterName, nameBufferSize, wideName.c_str(), nameBufferSize - 1);
                    outMonsterName[nameBufferSize - 1] = L'\0';
                }
                if (outTemperedLevel)
                {
                    *outTemperedLevel = result.temperedLevel;
                }
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DataBridge_MatchMonsterName failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall DataBridge_GetMonsterIconUrl(const wchar_t* monsterName, wchar_t* outUrl, int urlBufferSize)
    {
        try
        {
            std::string narrowName = WstringToUtf8(monsterName);
            auto url = DataBridge::GetMonsterDataManager()->GetMatchedMonsterIconUrl(narrowName);
            std::wstring wideUrl = Utf8ToWstring(url);
            wcsncpy_s(outUrl, urlBufferSize, wideUrl.c_str(), urlBufferSize - 1);
            outUrl[urlBufferSize - 1] = L'\0';
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DataBridge_GetMonsterIconUrl failed: %s"), e.what());
            if (outUrl && urlBufferSize > 0)
            {
                outUrl[0] = L'\0';
            }
        }
    }

    __declspec(dllexport) bool __stdcall PriorityQueue_Contains(const char* userId)
    {
        try
        {
            return DataBridge::GetPriorityQueueManager()->Contains(userId);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_Contains failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_Enqueue(const char* userId, long long timeStamp, bool priority, const wchar_t* userName, const wchar_t* monsterName, int guardLevel, int temperedLevel)
    {
        try
        {
            QueueNodeData node;
            node.userId = userId;
            node.timeStamp = timeStamp;
            node.priority = priority;
            node.userName = WstringToUtf8(userName);
            node.monsterName = WstringToUtf8(monsterName);
            node.guardLevel = guardLevel;
            node.temperedLevel = temperedLevel;
            DataBridge::GetPriorityQueueManager()->Enqueue(node);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_Enqueue failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_DequeueByIndex(int index)
    {
        try
        {
            DataBridge::GetPriorityQueueManager()->Dequeue(index);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_DequeueByIndex failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_SortQueue()
    {
        try
        {
            DataBridge::GetPriorityQueueManager()->SortQueue();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_SortQueue failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_Clear()
    {
        try
        {
            DataBridge::GetPriorityQueueManager()->Clear();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_Clear failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_SaveList()
    {
        try
        {
            DataBridge::GetPriorityQueueManager()->SaveList();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_SaveList failed: %s"), e.what());
        }
    }

    __declspec(dllexport) bool __stdcall PriorityQueue_LoadList()
    {
        try
        {
            return DataBridge::GetPriorityQueueManager()->LoadList();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_LoadList failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall PriorityQueue_GetAllNodes(OnQueueNodeCallback callback, void* userData)
    {
        try
        {
            auto nodes = DataBridge::GetPriorityQueueManager()->GetAllNodes();
            for (const auto& node : nodes)
            {
                std::wstring wuserId = Utf8ToWstring(node.userId);
                std::wstring wuserName = Utf8ToWstring(node.userName);
                std::wstring wmonsterName = Utf8ToWstring(node.monsterName);
                callback(
                    wuserId.c_str(),
                    node.timeStamp,
                    node.priority,
                    wuserName.c_str(),
                    wmonsterName.c_str(),
                    node.guardLevel,
                    node.temperedLevel,
                    userData);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("PriorityQueue_GetAllNodes failed: %s"), e.what());
        }
    }

    __declspec(dllexport) void __stdcall DanmuProcessor_ProcessDanmu(const char* jsonStr)
    {
        try
        {
            auto danmu = DataBridge::GetDanmuProcessor()->ParseDanmuJson(std::string(jsonStr));
            DataBridge::GetDanmuProcessor()->ProcessDanmu(danmu);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("DanmuProcessor_ProcessDanmu failed: %s"), e.what());
        }
    }

    static OnDanmuProcessedCallback g_danmuProcessedCallback = nullptr;
    static void* g_danmuProcessedUserData = nullptr;

    __declspec(dllexport) void __stdcall DataBridge_SetDanmuProcessedCallback(OnDanmuProcessedCallback callback, void* userData)
    {
        g_danmuProcessedCallback = callback;
        g_danmuProcessedUserData = userData;

        if (callback != nullptr)
        {
            DataBridge::GetDanmuProcessor()->AddDanmuProcessedListener([](const DanmuProcessResult& result) {
                if (g_danmuProcessedCallback != nullptr && result.addedToQueue)
                {
                    std::wstring wuserName = Utf8ToWstring(result.userName);
                    std::wstring wmonsterName = Utf8ToWstring(result.monsterName);
                    g_danmuProcessedCallback(wuserName.c_str(), wmonsterName.c_str(), g_danmuProcessedUserData);
                }
            });
        }
    }

    __declspec(dllexport) bool __stdcall CaptainCheckInModule_Initialize()
    {
        try
        {
#if !ONLY_ORDER_MONSTER
            if (!ProfileManager::Inst()->Init()) {
                LOG_ERROR(TEXT("CaptainCheckInModule_Initialize: ProfileManager::Init() failed"));
                return false;
            }

            DanmuProcessor::Inst()->Init();

            if (!CaptainCheckInModule::Inst()->Init()) {
                LOG_ERROR(TEXT("CaptainCheckInModule_Initialize: CaptainCheckInModule::Init() failed"));
                return false;
            }

            if (!RetroactiveCheckInModule::Inst()->Init()) {
                LOG_ERROR(TEXT("CaptainCheckInModule_Initialize: RetroactiveCheckInModule::Init() failed"));
                return false;
            }

            LOG_INFO(TEXT("CaptainCheckInModule_Initialize: initialization sequence completed successfully"));
#endif
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("CaptainCheckInModule_Initialize failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall CaptainCheckInModule_SetEnabled(bool enabled)
    {
        try
        {
#if !ONLY_ORDER_MONSTER
            CaptainCheckInModule::Inst()->SetEnabled(enabled);
#endif
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("CaptainCheckInModule_SetEnabled failed: %s"), e.what());
        }
    }

    __declspec(dllexport) bool __stdcall CaptainCheckInModule_IsEnabled()
    {
        try
        {
#if !ONLY_ORDER_MONSTER
            return CaptainCheckInModule::Inst()->IsEnabled();
#else
            return false;
#endif
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("CaptainCheckInModule_IsEnabled failed: %s"), e.what());
            return false;
        }
    }

    __declspec(dllexport) void __stdcall CaptainCheckInModule_SetTriggerWords(const char* words)
    {
        try
        {
#if !ONLY_ORDER_MONSTER
            CaptainCheckInModule::Inst()->SetTriggerWords(words);
#endif
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(TEXT("CaptainCheckInModule_SetTriggerWords failed: %s"), e.what());
        }
    }
}

OnAIReplyCallback g_aiReplyCallback = nullptr;
void* g_aiReplyUserData = nullptr;
std::mutex g_aiReplyMutex;

__declspec(dllexport) void __stdcall DataBridge_SetAIReplyCallback(OnAIReplyCallback callback, void* userData)
{
#if !ONLY_ORDER_MONSTER
    std::lock_guard<std::mutex> lock(g_aiReplyMutex);
    g_aiReplyCallback = callback;
    g_aiReplyUserData = userData;
#endif
}

extern "C" {
OnCheckinTTSPlayCallback g_checkinTTSPlayCallback = nullptr;
void* g_checkinTTSPlayUserData = nullptr;
}
std::mutex g_checkinTTSPlayMutex;

__declspec(dllexport) void __stdcall DataBridge_SetCheckinTTSPlayCallback(OnCheckinTTSPlayCallback callback, void* userData)
{
#if !ONLY_ORDER_MONSTER
    std::lock_guard<std::mutex> lock(g_checkinTTSPlayMutex);
    g_checkinTTSPlayCallback = callback;
    g_checkinTTSPlayUserData = userData;
#endif
}

__declspec(dllexport) void __stdcall TTSManager_GetCurrentProviderName(char* outBuffer, int bufferSize)
{
    try
    {
#if !ONLY_ORDER_MONSTER
        std::string name = TTSManager::Inst()->GetCurrentProviderName();
        if (outBuffer && bufferSize > 0)
        {
            strncpy_s(outBuffer, bufferSize, name.c_str(), _TRUNCATE);
        }
#else
        if (outBuffer && bufferSize > 0)
        {
            outBuffer[0] = '\0';
        }
#endif
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("TTSManager_GetCurrentProviderName failed: %s"), e.what());
        if (outBuffer && bufferSize > 0)
        {
            outBuffer[0] = '\0';
        }
    }
}
