#pragma once

#include "framework.h"
#include "resource.h"


// Manager Class, Singleton
class MonsterOrderWilds
{
	DECLARE_SINGLETON(MonsterOrderWilds)
public:
	// ��������Ϣ
	// �����ϣ��ԭ�����ڼ��������򷵻� WM_NULL
	UINT HandleHwndMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// ���������ӳɹ���Ļص�
	void OnBliveConnected();

	// ������������Ļص�
	void OnBliveDisconnected();

private:
	MonsterOrderWilds();

	// Tick
	void Tick();

	// ����WPF���淢������Ϣ------------------------
	void tickWPFCommand();

	// Hotkey���� ------------------------
private:
	bool AddHotKeys(HWND hWnd);
	std::vector<void (*)()> m_hotKeysMap;
	UINT8	__hotkeyid = 1;
};
