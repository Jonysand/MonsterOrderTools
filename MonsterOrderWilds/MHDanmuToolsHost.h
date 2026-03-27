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

// ToolsMain包装类
public ref class ToolsMainHost
{
public:
	static ToolsMainHost^ Inst();

	inline void OpenConfigWindow() { toolsMainInstance->OpenConfigWindow(); toolsMainInstance->SetWindowVersion(APP_VERSION); }

	inline void OnConnected() { toolsMainInstance->OnConnected(); };

	inline void OnDisconnected() { toolsMainInstance->OnDisconnected(); };

	inline void OnHotKeyLock() { toolsMainInstance->OnHotKeyLock(); }

	inline void OnReceiveRawMsg(const TString& rawStr) { 
		toolsMainInstance->OnReceivedRawMsg(ConvertUtils::TStringToString(rawStr));
	}

	inline std::string GetCommand() { 
		return std::move(ConvertUtils::StringToStdString(toolsMainInstance->GetCommand())); 
	}

	inline MonsterOrderWindows::MainConfig^ GetConfig() { return toolsMainInstance->GetConfigService()->Config; }

	inline bool RefreshMonsterList() { return toolsMainInstance->RefreshMonsterList(); }

private:
	ToolsMainHost();
	MonsterOrderWindows::ToolsMain^ toolsMainInstance;
private:
	static ToolsMainHost^ __instance;
};

#define GET_CONFIG(NAME) ToolsMainHost::Inst()->GetConfig()->NAME

// TTS引擎类型定义
#define TTS_ENGINE_MIMO "mimo"
#define TTS_ENGINE_SAPI "sapi"
#define TTS_ENGINE_AUTO "auto"

// 小米MiMo默认配置
#define MIMO_DEFAULT_VOICE "mimo_default"
#define MIMO_DEFAULT_AUDIO_FORMAT "mp3"
#define MIMO_DEFAULT_SPEED 1.0f
