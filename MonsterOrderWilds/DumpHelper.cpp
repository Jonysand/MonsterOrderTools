// DumpHelper.cpp
//------------------------------------------------------------------------------

#include "DumpHelper.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <string>

/*static*/ std::function<int()> DumpHelper::s_CrashCallback;

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

static std::string s_dumpFilename("test.dmp");
static DumpHelper::DUMP_TYPE s_dumpType = DumpHelper::MINI_DUMP;
 TString DumpHelper::s_ExtDumpInfo;

// MyExceptionFilter
//------------------------------------------------------------------------------
static LONG WINAPI MyExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// get MiniDumpWriteDump() function pointer
	HMODULE hDll = NULL;
	hDll = LoadLibraryA("dbghelp.dll");
	if (hDll == NULL)
	{
		char temp[2048];
		::GetCurrentDirectoryA(sizeof(temp), temp);

		std::string dbgpath = temp;
		dbgpath += "\\dbghelp.dll";

		hDll = LoadLibraryA(dbgpath.c_str());
		if (hDll == NULL)
			return EXCEPTION_CONTINUE_SEARCH;
	}

	MINIDUMPWRITEDUMP DumpFn = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
	if (DumpFn == NULL)
		return EXCEPTION_CONTINUE_SEARCH;

	// dump it
	HANDLE hDumpFile = CreateFileA(s_dumpFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;

	MINIDUMP_TYPE dtype = s_dumpType == DumpHelper::FULL_DUMP ? MiniDumpWithFullMemory : MiniDumpNormal;
	DumpFn(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, dtype, &loExceptionInfo, NULL, NULL);

	CloseHandle(hDumpFile);
	return EXCEPTION_EXECUTE_HANDLER;
}

// HandleCrash
//------------------------------------------------------------------------------
int DumpHelper::HandleCrash(void* ExceptionInfo)
{
	static bool crashHandled = false;
	if (crashHandled)
		return EXCEPTION_CONTINUE_SEARCH;
	else
		crashHandled = true;
	LPEXCEPTION_POINTERS pExceptionInfo = (LPEXCEPTION_POINTERS)ExceptionInfo;
	// Top bit in exception code is fatal exceptions. Report those but not other types.
	if ((pExceptionInfo->ExceptionRecord->ExceptionCode & 0x80000000L) == 0)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	// get MiniDumpWriteDump() function pointer
	HMODULE hDll = NULL;
	hDll = LoadLibraryA("dbghelp.dll");
	if (hDll == NULL)
	{
		char temp[2048];
		::GetCurrentDirectoryA(sizeof(temp), temp);

		std::string dbgpath = temp;
		dbgpath += "\\dbghelp.dll";

		hDll = LoadLibraryA(dbgpath.c_str());
		if (hDll == NULL)
			return EXCEPTION_CONTINUE_SEARCH;
	}

	MINIDUMPWRITEDUMP DumpFn = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
	if (DumpFn == NULL)
		return EXCEPTION_CONTINUE_SEARCH;

	// dump it
	HANDLE hDumpFile = CreateFileA(s_dumpFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = pExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;

	MINIDUMP_TYPE dtype = MiniDumpNormal;
	DumpFn(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, dtype, &loExceptionInfo, NULL, NULL);

	CloseHandle(hDumpFile);

	if (s_CrashCallback)
		return s_CrashCallback();

	return EXCEPTION_CONTINUE_SEARCH;
}

static void MyMakeCrash()
{
	int *a = 0;
	*a = 1;
}

/*static*/ void DumpHelper::SetCrashCallback(std::function<int()>&& d)
{
	s_CrashCallback = d; 
}

/*static*/ void DumpHelper::AppendDumpInfo(const TString& info)
{
	s_ExtDumpInfo.append(info);
}

static void __cdecl MyInvalidParameterHandler(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t)
{
	MyMakeCrash();
}

const TString& DumpHelper::GetExtDumpInfo()
{
	return s_ExtDumpInfo;
}

// DumpWorker
//------------------------------------------------------------------------------
void DumpHelper::Init(const char *dumpFilename, DumpHelper::DUMP_TYPE dtype)
{
	s_dumpFilename = dumpFilename;
	s_dumpType = dtype;

	SetUnhandledExceptionFilter(MyExceptionFilter);
	_set_invalid_parameter_handler(&MyInvalidParameterHandler);
	_set_purecall_handler(&MyMakeCrash);
	std::set_new_handler(MyMakeCrash);

	//_CrtSetReportMode(_CRT_ASSERT, 0);
}

//------------------------------------------------------------------------------