// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

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
#include <coroutine>
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

#define APP_VERSION 0

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


class Lock
{
public:
	inline void lock()
	{
		bool expectedWriting = false;
		while (!_lock.compare_exchange_weak(expectedWriting, true))
			std::this_thread::yield();
	}
	inline void unlock()
	{
		_lock.store(false);
	}
private:
	std::atomic<bool> _lock{ false };
};