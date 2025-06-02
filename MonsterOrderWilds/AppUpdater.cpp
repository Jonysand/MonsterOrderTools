#include "AppUpdater.h"
#include "MHDanmuToolsHost.h"
#include "WriteLog.h"

const TString TEMP_PATH = TEXT("Temp\\");
const TString CFG_PATH = TEXT("MonsterOrderWilds_configs\\");
const TString EXE_NAME = TEXT("MonsterOrderWilds.exe");

const static TString VERSION_API = TEXT("/get_version");
const static TString MONSTER_LIST_VERSION_API = TEXT("/get_monster_list_version");

namespace FileHelper
{
	const TCHAR* GetValidPath(const TCHAR* strPath)
	{
		// 重新格式化给定目录.
		static TString _p = TEXT("");
		_p = strPath;
		while ((_p[_p.size() - 1] == '\\') || (_p[_p.size() - 1] == '/'))
		{
			//_p = _p.Left(_p.GetLength() - 1);
			_p.erase(_p.size() - 1, 1);
		}
		_p += TEXT("\\");
		return _p.c_str();
	}

	bool CreateDir(const TCHAR* strPath)
	{
		if ((_taccess(strPath, 0)) == 0)
			return true;
		TCHAR DirName[MAX_PATH] = { 0 };
		TCHAR PathName[MAX_PATH] = { 0 };
		TCHAR* p = const_cast<TCHAR*>(strPath);
		TCHAR* q = DirName;
		TCHAR c = 0;
		LOG_DEBUG(TEXT("CreateDir [ %s ]"), strPath);
		_tcscpy_s(PathName, GetValidPath(strPath));
		p = PathName;
		if (((_tcscmp(p, TEXT(".\\")) == 0) || (_tcscmp(p, TEXT("..\\")) == 0)))
		{
			return true;
		}
		while (*p)
		{
			if (('\\' == *p) || ('/' == *p))
			{
				if (':' != *(p - 1))
				{
					// 如果指定目录不存在，则创建.
					if ((_taccess(DirName, 0)) == -1) {
						if (!::CreateDirectory(DirName, NULL)) {
							if (::GetLastError() != ERROR_ALREADY_EXISTS) {
								LOG_ERROR(TEXT("CreateDir 失败 [ %s ] %d"), DirName, ::GetLastError());
								return false;
							}
						}
					}
				}
				// 清除重复的斜杠
				while (*(p + 1))
				{
					c = *(p + 1);
					if (('\\' == c) || ('/' == c)) {
						p++;
					}
					else {
						break;
					}
				}
			}
			*q++ = *p++;
			*q = '\0';
		}
		// 如果指定目录不存在，则创建.
		if ((_taccess(DirName, 0)) == -1) {
			if (!::CreateDirectory(DirName, NULL)) {
				if (::GetLastError() != ERROR_ALREADY_EXISTS) {
					LOG_ERROR(TEXT("CreateDir 失败 [ %s ] %d"), DirName, ::GetLastError());
					return false;
				}
			}
		}
		return true;
	}

	bool CopyDirectory(const TCHAR* srcPath, const TCHAR* destPath)
	{
		LOG_DEBUG(TEXT("CopyDirectory [ %s ] to [ %s ]"), srcPath, destPath);

		// 如果指定源目录不存在，则退出.
		if ((_taccess(srcPath, 0)) == -1)
		{
			//	m_szLastError = "拷贝源目录不成功";
			LOG_ERROR(TEXT("CopyDirectory [ %s ] 拷贝源目录不成功"), srcPath);
			return false;
		}
		// 如果源目录和目的目录相同,则退出
		if (_tcsncmp(destPath, srcPath, _tcslen(srcPath)) == 0) {
			LOG_ERROR(TEXT("CopyDirectory 源目录和目的目录相同"));
			return false;
		}

		// 如果指定目的目录不存在，则创建.
		if (((_taccess(destPath, 0)) == -1) && !CreateDir(destPath))
		{
			//	m_szLastError = "创建目的目录不成功";
			LOG_ERROR(TEXT("CopyDirectory 创建目的目录不成功 [ %s ]"), destPath);
			return false;
		}

		TCHAR fileFound[MAX_PATH] = { 0 };
		TCHAR fileDest[MAX_PATH] = { 0 };
		TCHAR subSrcFolder[MAX_PATH] = { 0 };
		TCHAR subDestFolder[MAX_PATH] = { 0 };
		WIN32_FIND_DATA FileData;
		HANDLE hSearch;
		bool ret = true;

		_sntprintf_s(fileFound, sizeof(fileFound), _TRUNCATE, TEXT("%s\\*.*"), srcPath);
		hSearch = FindFirstFile(fileFound, &FileData);
		if (hSearch == INVALID_HANDLE_VALUE)
		{
			LOG_ERROR(TEXT("CopyDirectory 查找文件失败 [ %s ]"), srcPath);
			return false;
		}
		do
		{
			if (!((_tcscmp(FileData.cFileName, TEXT(".")) == 0) || (_tcscmp(FileData.cFileName, TEXT("..")) == 0)))
			{
				// 不是"." ".."路径
				if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// 是目录
					_sntprintf_s(subSrcFolder, sizeof(subSrcFolder), _TRUNCATE, TEXT("%s\\%s"), srcPath, FileData.cFileName);
					_sntprintf_s(subDestFolder, sizeof(subDestFolder), _TRUNCATE, TEXT("%s\\%s"), destPath, FileData.cFileName);
					if (!CopyDirectory(subSrcFolder, subDestFolder)) {
						LOG_ERROR(TEXT("CopyDirectory 子目录失败 [ %s ] to [ %s ]"), subSrcFolder, subDestFolder);
						ret = false;
					}
				}
				else	// 是文件
				{
					_sntprintf_s(fileFound, sizeof(fileFound), _TRUNCATE, TEXT("%s\\%s"), srcPath, FileData.cFileName);
					_sntprintf_s(fileDest, sizeof(fileDest), _TRUNCATE, TEXT("%s\\%s"), destPath, FileData.cFileName);
					if (!CopyFileEx(fileFound, fileDest, NULL, NULL, FALSE, 0)) {
						LOG_ERROR(TEXT("CopyDirectory 文件失败 [ %s ] to [ %s ]"), fileFound, fileDest);
						ret = false;
					}
				}
			}
		} while (FindNextFile(hSearch, &FileData));
		FindClose(hSearch);
		return ret;
	}
	
	bool DelFileEx(const TCHAR* strFileName)
	{
		DWORD dwAttrs = ::GetFileAttributes(strFileName);
		// 是文件，并且存在.
		if ((!(dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) && ((_taccess(strFileName, 0)) == 0))
		{
			if (dwAttrs & FILE_ATTRIBUTE_READONLY) {
				// 设置文件的属性.
				if (!::SetFileAttributes(strFileName, FILE_ATTRIBUTE_NORMAL))
				{
					//m_szLastError = "设置文件属性不成功";
					LOG_ERROR(TEXT("DelFileEx 设置文件属性不成功 [ %s ] %d"), strFileName, ::GetLastError());
					return false;
				}
			}
			if (!::DeleteFile(strFileName))
			{
				LOG_ERROR(TEXT("DelFileEx 删除文件不成功 [ %s ] %d"), strFileName, ::GetLastError());
				return false;
			}
		}
		return true;
	}

	bool EmptyDirectory(const TCHAR* strPath)
	{
		TCHAR fileFound[MAX_PATH] = { 0 };
		WIN32_FIND_DATA FileData;
		HANDLE hSearch;
		bool ret = true;
		_sntprintf_s(fileFound, sizeof(fileFound), _TRUNCATE, TEXT("%s\\*.*"), strPath);
		hSearch = FindFirstFile(fileFound, &FileData);
		if (hSearch == INVALID_HANDLE_VALUE)
			return false;
		do
		{
			if (!((_tcscmp(FileData.cFileName, TEXT(".")) == 0) || (_tcscmp(FileData.cFileName, TEXT("..")) == 0)))
			{
				// 不是"." ".."路径
				if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// 是目录
					TCHAR subFolder[MAX_PATH] = { 0 };
					_sntprintf_s(subFolder, sizeof(subFolder), _TRUNCATE, TEXT("%s\\%s"), strPath, FileData.cFileName);
					EmptyDirectory(subFolder);
					if (!::RemoveDirectory(subFolder))
						ret = false;
				}
				else	// 是文件
				{
					_sntprintf_s(fileFound, sizeof(fileFound), _TRUNCATE, TEXT("%s\\%s"), strPath, FileData.cFileName);
					if (!DelFileEx(fileFound))
						ret = false;
				}
			}

		} while (FindNextFile(hSearch, &FileData));

		FindClose(hSearch);
		return ret;
	}

	HANDLE StartProcess(const TString& strCmd, const TString& cur_dir, DWORD* err_code, int nRetry)
	{
		if (err_code) *err_code = 0;
		if (strCmd.empty() || strCmd == TEXT("0"))
		{
			return 0;	// 不启动新进程
		}
		// cur_dir: '0'表示采用当前路径，即为NULL.
		const _TCHAR* pDir = NULL;
		if (!cur_dir.empty() && cur_dir != TEXT("0"))
		{
			pDir = cur_dir.c_str();;
		}
		// start the process.
		if (nRetry <= 0) nRetry = 1;
		for (int i = 0; i < nRetry; ++i) {
			STARTUPINFO sinf;
			ZeroMemory(&sinf, sizeof(sinf));
			sinf.cb = sizeof(STARTUPINFO);
			PROCESS_INFORMATION pi;
			ZeroMemory(&pi, sizeof(pi));
			BOOL ret = CreateProcess(
				NULL,				// pointer to name of executable module
				(LPTSTR)strCmd.c_str(),	// lpCommandLine,
				NULL,				// pointer to process security attributes
				NULL,				// pointer to thread security attributes
				false,				// handle inheritance flag
				CREATE_NEW_CONSOLE,	// creation flags
				NULL,				// pointer to new environment block
				pDir,			    // pointer to current directory name
				&sinf,				// pointer to STARTUPINFO
				&pi 				// pointer to PROCESS_INFORMATION
			);
			if (ret) {
				return pi.hProcess;
			}
			if (i + 1 < nRetry) {
				DWORD err_code = ::GetLastError();
				LOG_ERROR(TEXT("create process errcode: %d"), err_code);
				::Sleep(100);
			}
		}
		if (err_code) {
			*err_code = ::GetLastError();
		}
		return 0;
	}
}

AppUpdater* AppUpdater::__Instance = nullptr;

AppUpdater* AppUpdater::Inst()
{
	if (!__Instance)
		__Instance = new AppUpdater();
	return __Instance;
}

void AppUpdater::Tick()
{
	// 网络协程
	if (!networkCoroutines.empty()) {
		auto it = networkCoroutines.begin();
		while (it != networkCoroutines.end()) {
			if (!it->resume())
				it = networkCoroutines.erase(it);
			else
				++it;
		}
	}
	if (!messageBoxQueue.empty()) {
		TString content = messageBoxQueue.begin()->first;
		TString title = messageBoxQueue.begin()->second;
		messageBoxQueue.pop_front();
		MessageBoxW(NULL, content.c_str(), title.c_str(), MB_OK);
	}
}

void AppUpdater::Destroy()
{
	if (__Instance)
	{
		delete __Instance;
		__Instance = nullptr;
	}
}

void AppUpdater::InitModule()
{
	InitWorkSpace();
	// 检查上一次是否已经下完了更新文件
	CheckUseUpdated();
	// 当前从Temp启动，拷贝文件后直接启动新的
	if (isInTemp)
		UpdateApply();
}

void AppUpdater::InitWorkSpace()
{
	TString filepath = TString(WriteLog::GetExeDirectory());
	size_t rPos = filepath.rfind(TEMP_PATH);
	TString srcDir;
	if (rPos != TString::npos)
	{
		isInTemp = true;
		srcDir = filepath.substr(0, rPos);
	}
	else
		srcDir = filepath.substr(0, rPos);
	::SetCurrentDirectory(srcDir.c_str());
	workSpace = srcDir + TEXT("\\");
	LOG_DEBUG(TEXT("InitWorkSpace: workSpace: %s"), workSpace.c_str());
}

void AppUpdater::CheckUseUpdated()
{
	TString tempExeDir = workSpace + TEMP_PATH;
	TString updatedFilePath = tempExeDir + TEXT("updated");
	TString exePath = tempExeDir + EXE_NAME;
	if (_taccess(updatedFilePath.c_str(), 0) == 0) // file exists
	{
		LOG_INFO(TEXT("Detected updated file, starting exe in Temp..."));
		DWORD err_code = 0;
		FileHelper::DelFileEx(updatedFilePath.c_str());
		if (!FileHelper::StartProcess(exePath, tempExeDir, &err_code, 3)) {
			LOG_ERROR(TEXT("Failed to start exe in Temp, error code: %u"), err_code);
		}
		else {
			LOG_INFO(TEXT("Started exe in Temp successfully."));
			exit(0);
		}
	}
	FileHelper::EmptyDirectory(tempExeDir.c_str());
}

void AppUpdater::StartFetchAppVersion()
{
	networkCoroutines.push_front(std::move(Network::MakeHttpsRequest(
		JONYSAND_URL,
		0,
		VERSION_API,
		TEXT("GET"),
		"",
		"",
		true,
		[this](const std::string& response) { OnVersionFetched(response); }
	)));
}

void AppUpdater::OnVersionFetched(const std::string& response)
{
	LOG_DEBUG(TEXT("OnVersionFetched: %s"), ProtoUtils::Decode(response).c_str());
	DWORD code = -1;
	json jsonResponse;
	try {
		jsonResponse = json::parse(response);
		int app_version = jsonResponse["app_version"].get<int>();
		if (app_version <= APP_VERSION)
		{
			messageBoxQueue.push_back({ TEXT("当前已是最新!") , TEXT("更新成功") });
			return;
		}
		json patchListRaw = jsonResponse["app_patch_list"];
		for (auto it = patchListRaw.begin(); it != patchListRaw.end(); ++it) {
			const TString& filepath = ProtoUtils::Decode(it.key());
			const std::string& value = it.value();
			TCHAR patch_file_url[MAX_PATH] = { 0 };
			_sntprintf_s(
				patch_file_url,
				_countof(patch_file_url),
				_TRUNCATE,
				TEXT("/get_app_file/%d/%s"),
				app_version,
				filepath.c_str()
			);
			patchList[filepath] = value;
			networkCoroutines.push_front(std::move(Network::MakeHttpsRequest(
				JONYSAND_URL,
				0,
				patch_file_url,
				TEXT("GET"),
				"",
				"",
				false,
				[this, filepath](const std::string& response) { OnFileFetched(filepath, response); }
			)));
		}
	}
	catch (const json::parse_error& e) {
		LOG_ERROR(TEXT("JSON parse error: %s"), ProtoUtils::Decode(response).c_str());
		return;
	}
}

void AppUpdater::OnFileFetched(const TString& filepath, const std::string& data)
{
	std::string md5data = hashpp::get::getHash(hashpp::ALGORITHMS::MD5, data);
	auto it = patchList.find(filepath);
	if (it == patchList.end()) {
		LOG_ERROR(TEXT("File path not found in patchList: %s"), filepath.c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
	if (md5data != it->second) {
		LOG_ERROR(TEXT("MD5 mismatch for %s: expected %s, got %s"),
			filepath.c_str(),
			ProtoUtils::Decode(it->second).c_str(),
			ProtoUtils::Decode(md5data).c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
	// Build full path: Temp\<filepath>
	TString tempDir = workSpace + TEMP_PATH;
	TString fullPath = tempDir + filepath;
	// Ensure directory exists
	size_t lastSlash = fullPath.find_last_of(TEXT("\\/"));
	if (lastSlash != TString::npos) {
		TString dir = fullPath.substr(0, lastSlash + 1);
		FileHelper::CreateDir(dir.c_str());
	}
	// Write file in binary mode
#ifdef UNICODE
	std::wstring wFullPath = fullPath;
	std::ofstream ofs(wFullPath, std::ios::binary);
#else
	std::ofstream ofs(fullPath, std::ios::binary);
#endif
	if (!ofs) {
		LOG_ERROR(TEXT("Failed to open file for writing: %s"), fullPath.c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
	ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
	ofs.close();

	if (++fileDownloaded == patchList.size())
	{
		TString updatedFilePath = workSpace + TEMP_PATH + TEXT("updated");
		FileHelper::CreateDir((workSpace + TEMP_PATH).c_str());
		// Write an empty file
#ifdef UNICODE
		std::wofstream updatedFile(updatedFilePath, std::ios::binary);
#else
		std::ofstream updatedFile(updatedFilePath, std::ios::binary);
#endif
		if (!updatedFile) {
			LOG_ERROR(TEXT("Failed to create updated file: %s"), updatedFilePath.c_str());
			messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		}
		else {
			updatedFile.close();
			messageBoxQueue.push_back({ TEXT("更新成功！请重启Monster Order Wilds!") , TEXT("更新成功") });
		}
	}
}

void AppUpdater::UpdateApply()
{
	// 如果当前是从Temp目录启动，则需要将自己拷贝到外面的根目录
	LOG_INFO(TEXT("Update --- Copy temp files"));
	//copy all temp files
	TString srcPath = workSpace + TEMP_PATH;
	if (!FileHelper::CopyDirectory(srcPath.c_str(), workSpace.c_str()))
	{
		LOG_ERROR(TEXT("Applay update failed!!!"));
		MessageBoxW(NULL, TEXT("更新失败，请重启工具，并找凳虎老哥！"), TEXT("更新失败"), MB_OK);
		exit(1);
	}
	LOG_INFO(TEXT("Update---Restart"));
	TString cmd = workSpace + EXE_NAME;
	DWORD err_code = 0;
	if (!FileHelper::StartProcess(cmd, workSpace, &err_code, 3)) {
		LOG_ERROR(TEXT("start exe error code:%u"), err_code);
		return;
	}
	else {
		LOG_INFO(TEXT("start exe success."));
		exit(0);
	}
}

void AppUpdater::StartFetchMonsterListVersion()
{
	networkCoroutines.push_front(std::move(Network::MakeHttpsRequest(
		JONYSAND_URL,
		0,
		MONSTER_LIST_VERSION_API,
		TEXT("GET"),
		"",
		"",
		true,
		[this](const std::string& response) { OnMonsterListVersionFetched(response); }
	)));
}

void AppUpdater::OnMonsterListVersionFetched(const std::string& response)
{
	LOG_DEBUG(TEXT("OnVersionFetched: %s"), ProtoUtils::Decode(response).c_str());
	DWORD code = -1;
	json jsonResponse;
	try {
		jsonResponse = json::parse(response);
		int monster_list_version = jsonResponse["monster_list_version"].get<int>();
		monsterListHash = jsonResponse["monster_list_hash"];
		TCHAR patch_file_url[MAX_PATH] = { 0 };
		_sntprintf_s(
			patch_file_url,
			_countof(patch_file_url),
			_TRUNCATE,
			TEXT("/get_monster_list/%d"),
			monster_list_version
		);
		networkCoroutines.push_front(std::move(Network::MakeHttpsRequest(
			JONYSAND_URL,
			0,
			patch_file_url,
			TEXT("GET"),
			"",
			"",
			false,
			[this](const std::string& response) { OnMonsterListFetched(response); }
		)));
	}
	catch (const json::parse_error& e) {
		LOG_ERROR(TEXT("JSON parse error: %s"), ProtoUtils::Decode(response).c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
}

void AppUpdater::OnMonsterListFetched(const std::string& data)
{
	std::string md5data = hashpp::get::getHash(hashpp::ALGORITHMS::MD5, data);
	if (md5data != monsterListHash) {
		LOG_ERROR(TEXT("MD5 mismatch for monster list: expected %s, got %s"),
			ProtoUtils::Decode(monsterListHash).c_str(),
			ProtoUtils::Decode(md5data).c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
	TString tempDir = workSpace + CFG_PATH;
	TString fullPath = tempDir + TEXT("monster_list.json");
	// Ensure directory exists
	size_t lastSlash = fullPath.find_last_of(TEXT("\\/"));
	if (lastSlash != TString::npos) {
		TString dir = fullPath.substr(0, lastSlash + 1);
		FileHelper::CreateDir(dir.c_str());
	}
	// Write file in binary mode
#ifdef UNICODE
	std::wstring wFullPath = fullPath;
	std::ofstream ofs(wFullPath, std::ios::binary);
#else
	std::ofstream ofs(fullPath, std::ios::binary);
#endif
	if (!ofs) {
		LOG_ERROR(TEXT("Failed to open file for writing: %s"), fullPath.c_str());
		messageBoxQueue.push_back({ TEXT("更新失败！请重试或联系凳虎老哥!") , TEXT("更新失败") });
		return;
	}
	ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
	ofs.close();
	if (ToolsMainHost::Inst()->RefreshMonsterList())
		messageBoxQueue.push_back({ TEXT("怪物列表更新成功！") , TEXT("更新成功") });
	else
		messageBoxQueue.push_back({ TEXT("怪物列表更新失败，请找凳虎老哥！"), TEXT("更新失败") });
}