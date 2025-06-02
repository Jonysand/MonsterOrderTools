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

	// 正在进行的http协程
	std::list<Network::NetworkCoroutine> networkCoroutines;

	// 协程里MessageBoxW，ok后会直接挂掉，用个队列延迟弹窗
	std::list<std::pair<TString, TString>> messageBoxQueue;

	// 文件列表
	std::map<TString, std::string> patchList;
	// 已下载的文件数量
	uint8_t fileDownloaded{ 0 };
	// 远端monster list md5
	std::string monsterListHash{ "" };

	TString workSpace{ TEXT("\\") };
	bool isInTemp{ false };

	static AppUpdater* __Instance;
};