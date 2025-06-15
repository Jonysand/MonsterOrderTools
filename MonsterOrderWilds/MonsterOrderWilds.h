#pragma once

#include "framework.h"
#include "resource.h"


// Manager Class, Singleton
class MonsterOrderWilds
{
	DECLARE_SINGLETON(MonsterOrderWilds)
public:
	// 处理窗口消息
	// 如果不希望原生窗口继续处理则返回 WM_NULL
	UINT HandleHwndMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// 服务器链接成功后的回调
	void OnBliveConnected();

	// 服务器断连后的回调
	void OnBliveDisconnected();

private:
	MonsterOrderWilds();

	// Tick
	void Tick();

	// 处理WPF界面发来的消息------------------------
	void tickWPFCommand();

	// Hotkey处理 ------------------------
private:
	bool AddHotKeys(HWND hWnd);
	std::vector<void (*)()> m_hotKeysMap;
	UINT8	__hotkeyid = 1;
};
