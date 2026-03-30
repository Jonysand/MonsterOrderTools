#pragma once
#include "framework.h"

namespace WriteLog
{
	const TCHAR* GetExeDirectory();
	enum LogLevel
	{
		LogLevel_ERROR = 0,			// 错误日志
		LogLevel_WARNING = 1,		// 警告日志
		LogLevel_INFO = 2,			// 一般信息日志
		LogLevel_DEBUG = 3,			// 调试日志
	};
	void WriteLog(LogLevel level, const TCHAR* msg, ...);
	void RecordHistory(const TCHAR* msg);
}
TString GetLastErrorAsTString();

static inline std::wstring Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

static inline std::string WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}

#define LOGW_ERROR(EXPR, ...)		{ wchar_t __buf__[4096]; _snwprintf_s(__buf__, 4096, _TRUNCATE, EXPR, __VA_ARGS__); WriteLog::WriteLog(WriteLog::LogLevel_ERROR, __buf__); }
#define LOGW_WARNING(EXPR, ...)		{ wchar_t __buf__[4096]; _snwprintf_s(__buf__, 4096, _TRUNCATE, EXPR, __VA_ARGS__); WriteLog::WriteLog(WriteLog::LogLevel_WARNING, __buf__); }
#define LOGW_INFO(EXPR, ...)		{ wchar_t __buf__[4096]; _snwprintf_s(__buf__, 4096, _TRUNCATE, EXPR, __VA_ARGS__); WriteLog::WriteLog(WriteLog::LogLevel_INFO, __buf__); }
#if _DEBUG
#define LOGW_DEBUG(EXPR, ...)		{ wchar_t __buf__[4096]; _snwprintf_s(__buf__, 4096, _TRUNCATE, EXPR, __VA_ARGS__); WriteLog::WriteLog(WriteLog::LogLevel_DEBUG, __buf__); }
#else
#define LOGW_DEBUG(EXPR, ...)
#endif

#define LOG_ERROR(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_ERROR, EXPR, __VA_ARGS__)
#define LOG_WARNING(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_WARNING, EXPR, __VA_ARGS__)
#define LOG_INFO(EXPR, ...)			WriteLog::WriteLog(WriteLog::LogLevel_INFO, EXPR, __VA_ARGS__)
#if _DEBUG
#define LOG_DEBUG(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_DEBUG, EXPR, __VA_ARGS__)
#else
#define LOG_DEBUG(EXPR, ...)
#endif

#define RECORD_HISTORY(msg)			WriteLog::RecordHistory(msg);
