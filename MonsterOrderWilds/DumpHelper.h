// DumpHelper.h
//------------------------------------------------------------------------------
#pragma once
#include "framework.h"

class DumpHelper
{
public:
	enum DUMP_TYPE { FULL_DUMP, MINI_DUMP };
	
public:
	static void Init(const char *dumpFilename, DUMP_TYPE dtype);
	static int  HandleCrash(void* ExceptionInfo);
	static void SetCrashCallback(std::function<int()>&& d);
	static void AppendDumpInfo(const TString& info);
	static const TString& GetExtDumpInfo();
private:
	static std::function<int()> s_CrashCallback;
	static TString s_ExtDumpInfo;
};

//------------------------------------------------------------------------------