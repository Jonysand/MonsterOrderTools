#include "MonsterOrderWilds.h"
#include "MHDanmuToolsHost.h"
#include "BliveManager.h"
#include "AppUpdater.h"
#include "CredentialsConsts.h"
#include "Network.h"
#include "WriteLog.h"

#define WIDGETID_IDINPUT	1001
#define WIDGETID_IDCONFIRM	1002

MonsterOrderWilds* MonsterOrderWilds::__Instance = nullptr;

MonsterOrderWilds::MonsterOrderWilds()
{
	LOG_INFO(TEXT("--------------------MonsterOrderWilds start! v %d -------------------------"), APP_VERSION);
}

MonsterOrderWilds* MonsterOrderWilds::Inst()
{
	if (!__Instance)
		__Instance = new MonsterOrderWilds();
	return __Instance;
}

UINT MonsterOrderWilds::HandleHwndMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCCREATE:
	{
		AppUpdater::Inst()->InitModule();
		ToolsMainHost::Inst()->OpenConfigWindow();
		break;
	}
	case WM_TIMER:
		if (wParam == TIMER_ID)
			//Tick coroutine
			Tick();
		break;
	case WM_CREATE:
	{
		AddHotKeys(hWnd);
		BliveManager::Inst()->AddListener_OnBliveConnected([this]() { OnBliveConnected(); });
		break;
	}
	case WM_HOTKEY:
	{
		int id = (int)wParam;
		if (id < __hotkeyid)
		{
			m_hotKeysMap[id]();
		}
		break;
	}
	case WM_DESTROY:
	{
		// 关闭弹幕服务器
		BliveManager::Inst()->SetConnected(false);
		BliveManager::Inst()->Destroy();
		break;
	}
	default:
		break;
	}
	return message;
}

void MonsterOrderWilds::Tick()
{
	// BliveManager tick
	BliveManager::Inst()->Tick();
	// AppUpdater tick
	AppUpdater::Inst()->Tick();
	// Commands from WPF
	tickWPFCommand();
}

std::vector<std::string> SplitCommand(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delimiter)) {
		tokens.push_back(item);
	}
	return std::move(tokens);
}

void MonsterOrderWilds::tickWPFCommand()
{
	const auto& command = SplitCommand(ToolsMainHost::Inst()->GetCommand(), ':');
	if (command.empty()) return;
	// 确认身份码，连接弹幕服务器
	if (strcmp(command[0].c_str(), "ConfirmIDCode") == 0)
	{
		if (!BliveManager::Inst()->IsConnected())
			BliveManager::Inst()->Start(command[1]);
	}
	// LOG
	if (strcmp(command[0].c_str(), "Log") == 0)
		LOG_INFO(TEXT("[GUI] %s"), command[1].c_str());
	// Update
	if (strcmp(command[0].c_str(), "Update") == 0)
		AppUpdater::Inst()->StartFetchAppVersion();
	// Update
	if (strcmp(command[0].c_str(), "UpdateList") == 0)
		AppUpdater::Inst()->StartFetchMonsterListVersion();
	// 退出主程序
	if (strcmp(command[0].c_str(), "Exit") == 0)
	{
		BliveManager::Inst()->SetConnected(false);
		BliveManager::Inst()->Destroy();
		PostQuitMessage(0);
	}
}

void MonsterOrderWilds::OnBliveConnected()
{
	ToolsMainHost::Inst()->OnConnected();
}

void MonsterOrderWilds::OnBliveDisconnected()
{
	BliveManager::Inst()->Start();
	ToolsMainHost::Inst()->OnDisconnected();
}

bool MonsterOrderWilds::AddHotKeys(HWND hWnd)
{
	m_hotKeysMap.clear();
	m_hotKeysMap.push_back(nullptr);

	// start from 1
	// alt + ,
	UINT curHotkey = __hotkeyid++;
	if (RegisterHotKey(
		hWnd,
		curHotkey,
		MOD_ALT | MOD_NOREPEAT,
		VK_OEM_COMMA))
	{
		m_hotKeysMap.push_back([]() { ToolsMainHost::Inst()->OnHotKeyLock(); });
	}
	if (m_hotKeysMap.size() != __hotkeyid)
		return false;
	m_hotKeysMap.shrink_to_fit();
	return true;
}
