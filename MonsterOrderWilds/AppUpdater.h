#pragma once
#include "framework.h"
#include "Network.h"


class AppUpdater
{
public:
	static AppUpdater* Inst();
	void InitModule();
	void StartFetchAppVersion();
	void StartFetchMonsterListVersion();

	void Tick();
	void Destroy();
private:
	void InitWorkSpace();
	void CheckUseUpdated();
	void OnVersionFetched(const std::string& response);
	void OnFileFetched(const TString& filepath, const std::string& data);
	void UpdateApply();

	void OnMonsterListVersionFetched(const std::string& response);
	void OnMonsterListFetched(const std::string& data);

	// ���ڽ��е�httpЭ��
	std::list<Network::NetworkCoroutine> networkCoroutines;

	// Э����MessageBoxW��ok���ֱ�ӹҵ����ø������ӳٵ���
	std::list<std::pair<TString, TString>> messageBoxQueue;

	// �ļ��б�
	std::map<TString, std::string> patchList;
	// �����ص��ļ�����
	uint8_t fileDownloaded{ 0 };
	// Զ��monster list md5
	std::string monsterListHash{ "" };

	TString workSpace{ TEXT("\\") };
	bool isInTemp{ false };

	static AppUpdater* __Instance;
};