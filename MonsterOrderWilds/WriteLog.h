#pragma once
#include "framework.h"

namespace WriteLog
{
	const TCHAR* GetExeDirectory();
	enum LogLevel
	{
		LogLevel_ERROR = 0,			// ������־
		LogLevel_WARNING = 1,		// ������־
		LogLevel_INFO = 2,			// һ����Ϣ��־
		LogLevel_DEBUG = 3,			// ������־
	};
	void WriteLog(LogLevel level, const TCHAR* msg, ...);
	void ClearLog(void);
}
TString GetLastErrorAsTString();

#define LOG_ERROR(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_ERROR, EXPR, __VA_ARGS__)
#define LOG_WARNING(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_WARNING, EXPR, __VA_ARGS__)
#define LOG_INFO(EXPR, ...)			WriteLog::WriteLog(WriteLog::LogLevel_INFO, EXPR, __VA_ARGS__)
#if _DEBUG
#define LOG_DEBUG(EXPR, ...)		WriteLog::WriteLog(WriteLog::LogLevel_DEBUG, EXPR, __VA_ARGS__)
#else
#define LOG_DEBUG(EXPR, ...)
#endif
