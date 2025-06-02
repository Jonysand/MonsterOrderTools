#include "MHDanmuToolsHost.h"
#include "WriteLog.h"

using namespace System;

ToolsMainHost^ ToolsMainHost::Inst()
{
	if (!__instance)
		__instance = gcnew ToolsMainHost();
	return __instance;
}

ToolsMainHost::ToolsMainHost()
{
	toolsMainInstance = gcnew MonsterOrderWindows::ToolsMain();
	toolsMainInstance->Inited();
}