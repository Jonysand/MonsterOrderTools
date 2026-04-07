// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

// Suppress codecvt deprecation warning
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <winhttp.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <map>
#include <ctime>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <exception>
#include <vector>
#include <queue>
#include <functional>
#include <codecvt>
#include <locale>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <chrono>

// json decode
#include "nlohmann/json.hpp"
using json = nlohmann::json;

// md5 & sha256
#include "hashpp.h"

// 小米MiMo TTS 编译时开关
// 设为 0 可完全排除小米MiMo相关代码（适用于无网络环境或不需要小米MiMo的场景）
#ifndef USE_MIMO_TTS
#define USE_MIMO_TTS 1
#endif

// 本地测试宏 - 跳过舰长检测，方便本地测试 captain-checkin-ai-reply 功能
#ifndef TEST_CAPTAIN_REPLY_LOCAL
#define TEST_CAPTAIN_REPLY_LOCAL 0
#endif

#define APP_VERSION 15

#ifdef UNICODE
typedef std::wstring TString;
#else
typedef std::string TString;
#endif

#ifdef _UNICODE
#define VSPRINTF_FUNC vswprintf_s
#define PRINTF_FUNC wprintf
#else
#define VSPRINTF_FUNC vsprintf_s
#define PRINTF_FUNC printf
#endif


#define TIMER_ID 1
#define TIMER_INTERVAL 100

// Singleton

#define DECLARE_SINGLETON(CLASS) public: \
									static CLASS* Inst(); \
									void Destroy(); \
									static std::atomic<bool>& GetDestroyingFlag(); \
								private: \
									static CLASS* __Instance;
#define DEFINE_SINGLETON(CLASS) CLASS* CLASS::__Instance = nullptr; \
								CLASS* CLASS::Inst(){ \
									if (!__Instance) __Instance = new CLASS(); \
									return __Instance; } \
								void CLASS::Destroy() { \
									GetDestroyingFlag().store(true); \
									if (__Instance) { delete __Instance; __Instance = nullptr; } \
								} \
								std::atomic<bool>& CLASS::GetDestroyingFlag() { \
									static std::atomic<bool> __Destroying{false}; \
									return __Destroying; \
								}

class Lock
{
public:
	inline void lock()
	{
		bool expectedWriting = false;
		while (!_lock.compare_exchange_weak(expectedWriting, true)) {
			expectedWriting = false;
			std::this_thread::yield();
		}
	}
	inline void unlock()
	{
		_lock.store(false);
	}
private:
	std::atomic<bool> _lock{ false };
};