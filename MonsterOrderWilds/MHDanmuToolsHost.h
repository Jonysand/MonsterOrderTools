#pragma once

#include "framework.h"

using namespace System;
using namespace System::Windows;
using namespace System::Windows::Documents;
using namespace System::Threading;
using namespace System::Windows::Controls;
using namespace System::Windows::Media;
using namespace System::Runtime::InteropServices;
#include <msclr/marshal_cppstd.h>

namespace ConvertUtils
{
	inline String^ TStringToString(const TString& input)
	{
#ifdef UNICODE
		return gcnew String(input.c_str());
#else
		return msclr::interop::marshal_as<String^>(input);
#endif
	}

	inline std::string StringToStdString(System::String^ input)
	{
		return msclr::interop::marshal_as<std::string>(input);
	}
}

// ToolsMain°ü×°Àà
public ref class ToolsMainHost
{
public:
	static ToolsMainHost^ Inst();

	inline void OpenConfigWindow() { toolsMainInstance->OpenConfigWindow(); }

	inline void OnConnected() { toolsMainInstance->OnConnected(); };

	inline void OnDisconnected() { toolsMainInstance->OnDisconnected(); };

	inline void OnHotKeyLock() { toolsMainInstance->OnHotKeyLock(); }

	inline void OnReceiveRawMsg(const TString& rawStr) { 
		toolsMainInstance->OnReceivedRawMsg(ConvertUtils::TStringToString(rawStr));
	}

	inline std::string GetCommand() { 
		return std::move(ConvertUtils::StringToStdString(toolsMainInstance->GetCommand())); 
	}

	inline bool RefreshMonsterList() { return toolsMainInstance->RefreshMonsterList(); }

private:
	ToolsMainHost();
	MonsterOrderWindows::ToolsMain^ toolsMainInstance;
private:
	static ToolsMainHost^ __instance;
};

