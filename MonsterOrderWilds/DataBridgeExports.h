#pragma once

#include "framework.h"
#include "WriteLog.h"
#include "DataBridge.h"
#include "PriorityQueueManager.h"
#include "DanmuProcessor.h"

typedef void(__stdcall* OnDanmuProcessedCallback)(const wchar_t* userName, const wchar_t* monsterName, void* userData);
typedef void(__stdcall* OnQueueNodeCallback)(const wchar_t* userId, long long timeStamp, bool priority, const wchar_t* userName, const wchar_t* monsterName, int guardLevel, int temperedLevel, void* userData);
typedef void(__stdcall* OnAIReplyCallback)(const wchar_t* username, const wchar_t* content, void* userData);
typedef void(__stdcall* OnCheckinTTSPlayCallback)(const wchar_t* username, const wchar_t* content, void* userData);

extern OnAIReplyCallback g_aiReplyCallback;
extern void* g_aiReplyUserData;

#ifdef __cplusplus
extern "C" {
#endif

// Config字段类型 (与C#中ConfigFieldType枚举对应)
#define CONFIG_TYPE_STRING 0
#define CONFIG_TYPE_BOOL 1
#define CONFIG_TYPE_INT 2
#define CONFIG_TYPE_FLOAT 3
#define CONFIG_TYPE_DOUBLE 4

    __declspec(dllexport) bool __stdcall DataBridge_Initialize();
    __declspec(dllexport) void __stdcall DataBridge_Shutdown();

    __declspec(dllexport) bool __stdcall DataBridge_IsMonsterDataLoaded();
    __declspec(dllexport) bool __stdcall DataBridge_MatchMonsterName(const wchar_t* inputText, wchar_t* outMonsterName, int nameBufferSize, int* outTemperedLevel);
    __declspec(dllexport) void __stdcall DataBridge_GetMonsterIconUrl(const wchar_t* monsterName, wchar_t* outUrl, int urlBufferSize);

    __declspec(dllexport) void __stdcall Config_SetValue(const char* key, const char* value, int type);
    __declspec(dllexport) void __stdcall Config_Save();
    __declspec(dllexport) void __stdcall Config_GetString(const char* key, char* outValue, int bufferSize);
    __declspec(dllexport) void __stdcall Config_GetBool(const char* key, bool* outValue);
    __declspec(dllexport) void __stdcall Config_GetInt(const char* key, int* outValue);
    __declspec(dllexport) void __stdcall Config_GetFloat(const char* key, float* outValue);
    __declspec(dllexport) void __stdcall Config_GetDouble(const char* key, double* outValue);

    __declspec(dllexport) bool __stdcall PriorityQueue_Contains(const char* userId);
    __declspec(dllexport) void __stdcall PriorityQueue_Enqueue(const char* userId, long long timeStamp, bool priority, const wchar_t* userName, const wchar_t* monsterName, int guardLevel, int temperedLevel);
    __declspec(dllexport) void __stdcall PriorityQueue_DequeueByIndex(int index);
    __declspec(dllexport) void __stdcall PriorityQueue_SortQueue();
    __declspec(dllexport) void __stdcall PriorityQueue_Clear();
    __declspec(dllexport) void __stdcall PriorityQueue_SaveList();
    __declspec(dllexport) bool __stdcall PriorityQueue_LoadList();
    __declspec(dllexport) void __stdcall PriorityQueue_GetAllNodes(OnQueueNodeCallback callback, void* userData);

    __declspec(dllexport) void __stdcall DanmuProcessor_ProcessDanmu(const char* jsonStr);

    __declspec(dllexport) void __stdcall DataBridge_SetDanmuProcessedCallback(OnDanmuProcessedCallback callback, void* userData);

    __declspec(dllexport) void __stdcall DataBridge_SetAIReplyCallback(OnAIReplyCallback callback, void* userData);

__declspec(dllexport) void __stdcall DataBridge_SetCheckinTTSPlayCallback(OnCheckinTTSPlayCallback callback, void* userData);

#ifdef __cplusplus
}
#endif
