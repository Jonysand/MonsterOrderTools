#pragma once

#include "framework.h"
#include <msclr/marshal_cppstd.h>

namespace ConvertUtils
{
	inline System::String^ TStringToString(const TString& input)
	{
#ifdef UNICODE
		return gcnew System::String(input.c_str());
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

	inline MonsterOrderWindows::MainConfig^ GetConfig() { return toolsMainInstance->GetConfig(); }

	inline bool RefreshMonsterList() { return toolsMainInstance->RefreshMonsterList(); }

private:
	ToolsMainHost();
	MonsterOrderWindows::ToolsMain^ toolsMainInstance;
private:
	static ToolsMainHost^ __instance;
};

#define GET_CONFIG(NAME) ToolsMainHost::Inst()->GetConfig()->NAME
