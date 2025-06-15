#include "WriteLog.h"
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <io.h>

#pragma warning(disable : 4793) // disable warning C4793: 'void __cdecl _vsntprintf_s(wchar_t *,unsigned __int64,const wchar_t *,...)' : function compiled as native

TString GetLastErrorAsTString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return TEXT("No error");

	LPWSTR messageBuffer = nullptr;

	// Format the error message from the system
	size_t size = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&messageBuffer, 0, NULL);

	TString message(messageBuffer, size);

	// Free the buffer allocated by FormatMessage
	LocalFree(messageBuffer);

	return message;
}

namespace WriteLog
{
	FILE* g_fp = NULL;
	Lock  writtingLock;

	const TCHAR* GetExeDirectory()
	{
		static TCHAR buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);

		TCHAR* lastBackslash = _tcsrchr(buffer, _T('\\'));
		if (lastBackslash != NULL)
		{
			*(lastBackslash + 1) = _T('\0');
		}

		return buffer;
	}

	static const TCHAR* strLogType[] = { TEXT("ERROR"), TEXT("WARNING"), TEXT("INFO"), TEXT("DEBUG") };

	void WriteLog(LogLevel level, const TCHAR* msg, ...)
	{
		writtingLock.lock();

		__try {
			if (!g_fp) {
				const TCHAR* currentDir = GetExeDirectory();
				TCHAR logPath[MAX_PATH] = { 0 };
				_tcscpy_s(logPath, currentDir);
				_tcscat_s(logPath, TEXT("log.txt"));

				// Open the file in binary mode to ensure UTF-8 BOM is written
				_tfopen_s(&g_fp, logPath, TEXT("ab"));
				if (g_fp) {
					// Check if the file is empty and write the UTF-8 BOM if necessary
					if (_filelength(_fileno(g_fp)) == 0) {
						const unsigned char utf8BOM[] = { 0xEF, 0xBB, 0xBF };
						fwrite(utf8BOM, sizeof(utf8BOM), 1, g_fp);
					}
					fclose(g_fp);
				}

				// Reopen the file in append mode for text writing
				_tfopen_s(&g_fp, logPath, TEXT("a, ccs=UTF-8"));
			}

			if (g_fp)
			{
				TCHAR strLogTag[512] = { 0 };
				SYSTEMTIME st;
				GetLocalTime(&st);

				TCHAR strLogContext[1024] = { 0 };
				va_list arg_prt;
				va_start(arg_prt, msg);
				_vsntprintf_s(strLogContext, sizeof(strLogContext) / sizeof(TCHAR), _TRUNCATE, msg, arg_prt);
				va_end(arg_prt);

				_sntprintf_s(strLogTag, sizeof(strLogTag) / sizeof(TCHAR), _TRUNCATE, TEXT("[%04d-%02d-%02d %02d:%02d:%02d]:[%s]"),
					st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, strLogType[level]);
				_ftprintf(g_fp, TEXT("%s %s\n"), strLogTag, strLogContext);
				fflush(g_fp);
				fclose(g_fp);
				g_fp = NULL;
			}
		}
		__finally {
			writtingLock.unlock();
		}
	}
}