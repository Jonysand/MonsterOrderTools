#include "TTSCacheManager.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "ConfigManager.h"
#include <Shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma comment(lib, "Shlwapi.lib")

DEFINE_SINGLETON(TTSCacheManager)

std::wstring TTSCacheManager::GetCacheBaseDir() const {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.rfind(L'\\');
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    return exeDir + L"\\TempAudio";
}

std::string TTSCacheManager::GetTodayDateStr() const {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
    return std::string(buffer);
}

std::wstring TTSCacheManager::GetTodayCacheDir() const {
    std::string today = GetTodayDateStr();
    return GetCacheBaseDir() + L"\\" + utf8_to_wstring(today);
}

std::string TTSCacheManager::GetContentPrefix(const std::string& text) const {
    std::wstring wtext = utf8_to_wstring(text);
    
    std::wstring searchTarget = L" 说：";
    size_t pos = wtext.find(searchTarget);
    if (pos != std::wstring::npos) {
        std::wstring username = wtext.substr(0, pos);
        size_t contentStart = pos + searchTarget.length();
        std::wstring afterSpeaker = wtext.substr(contentStart);
        std::wstring first5 = afterSpeaker.substr(0, std::min<size_t>(5, afterSpeaker.length()));
        return wstring_to_utf8(username + L"_" + first5);
    }
    
    std::wstring first5 = wtext.substr(0, std::min<size_t>(5, wtext.length()));
    return wstring_to_utf8(first5);
}

bool TTSCacheManager::SaveCachedAudio(const std::string& text, const std::vector<uint8_t>& audioData) {
    std::wstring todayDir = GetTodayCacheDir();
    if (!std::filesystem::exists(todayDir)) {
        if (!CreateDirectoryW(todayDir.c_str(), NULL)) {
            DWORD dirError = ::GetLastError();
            if (dirError != ERROR_ALREADY_EXISTS) {
                LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache directory"));
                return false;
            }
        }
    }
    
    std::string prefix = GetContentPrefix(text);
    
    int64_t timestamp = GetTickCount64();
    std::wstring fileName = utf8_to_wstring(prefix) + L"_" + std::to_wstring(timestamp) + L".mp3";
    std::wstring filePath = todayDir + L"\\" + fileName;
    
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache file"));
        return false;
    }
    
    DWORD bytesWritten;
    LOG_INFO(TEXT("TTSCacheManager: Writing %d bytes to cache file"), (int)audioData.size());
    if (!WriteFile(hFile, audioData.data(), static_cast<DWORD>(audioData.size()), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        LOG_ERROR(TEXT("TTSCacheManager: Failed to write cache data"));
        return false;
    }
    
    LOG_INFO(TEXT("TTSCacheManager: Wrote %d bytes to %s"), (int)bytesWritten, filePath.c_str());
    CloseHandle(hFile);
    LOG_INFO(TEXT("TTSCacheManager: Cached audio to %s"), filePath.c_str());
    return true;
}

bool TTSCacheManager::SaveCachedAudioWithPrefix(const std::string& text, const std::vector<uint8_t>& audioData, const std::string& prefix) {
    std::wstring todayDir = GetTodayCacheDir();
    if (!std::filesystem::exists(todayDir)) {
        if (!CreateDirectoryW(todayDir.c_str(), NULL)) {
            DWORD dirError = ::GetLastError();
            if (dirError != ERROR_ALREADY_EXISTS) {
                LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache directory"));
                return false;
            }
        }
    }

    int64_t timestamp = GetTickCount64();
    std::wstring fileName = utf8_to_wstring(prefix) + L"_" + std::to_wstring(timestamp) + L".mp3";
    std::wstring filePath = todayDir + L"\\" + fileName;

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache file with prefix"));
        return false;
    }

    DWORD bytesWritten;
    LOG_INFO(TEXT("TTSCacheManager: Writing %d bytes to cache file with prefix %hs"), (int)audioData.size(), prefix.c_str());
    if (!WriteFile(hFile, audioData.data(), static_cast<DWORD>(audioData.size()), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        LOG_ERROR(TEXT("TTSCacheManager: Failed to write cache data with prefix"));
        return false;
    }

    CloseHandle(hFile);
    LOG_INFO(TEXT("TTSCacheManager: Cached audio with prefix to %s"), filePath.c_str());
    return true;
}

bool TTSCacheManager::SaveCheckinAudio(const std::string& username, const std::vector<uint8_t>& audioData, int64_t timestamp) {
    std::wstring todayDir = GetTodayCacheDir();
    if (!std::filesystem::exists(todayDir)) {
        if (!CreateDirectoryW(todayDir.c_str(), NULL)) {
            DWORD dirError = ::GetLastError();
            if (dirError != ERROR_ALREADY_EXISTS) {
                LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache directory"));
                return false;
            }
        }
    }

    std::wstring fileName = L"打卡_" + utf8_to_wstring(username) + L"_" + std::to_wstring(timestamp) + L".mp3";
    std::wstring filePath = todayDir + L"\\" + fileName;

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(TEXT("TTSCacheManager: Failed to create checkin cache file"));
        return false;
    }

    DWORD bytesWritten;
    LOG_INFO(TEXT("TTSCacheManager: Writing %d bytes to checkin cache file"), (int)audioData.size());
    if (!WriteFile(hFile, audioData.data(), static_cast<DWORD>(audioData.size()), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        LOG_ERROR(TEXT("TTSCacheManager: Failed to write checkin cache data"));
        return false;
    }

    CloseHandle(hFile);
    LOG_INFO(TEXT("TTSCacheManager: Cached checkin audio to %s"), filePath.c_str());
    return true;
}

void TTSCacheManager::CleanupOldCache(int daysToKeep) {
    std::wstring baseDir = GetCacheBaseDir();
    if (!std::filesystem::exists(baseDir)) {
        return;
    }
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    int64_t cutoffTime = 0;
    
    FILETIME ftNow;
    SystemTimeToFileTime(&st, &ftNow);
    ULARGE_INTEGER ulNow = {ftNow.dwLowDateTime, ftNow.dwHighDateTime};
    ulNow.QuadPart -= (static_cast<ULONGLONG>(daysToKeep) * 24 * 60 * 60 * 10000000);
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((baseDir + L"\\*.*").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring dirName = findData.cFileName;
            if (dirName == L"." || dirName == L"..") continue;
            
            FILETIME ftCreate = findData.ftCreationTime;
            ULARGE_INTEGER ulCreate = {ftCreate.dwLowDateTime, ftCreate.dwHighDateTime};
            
            if (ulCreate.QuadPart < ulNow.QuadPart) {
                std::wstring dirPath = baseDir + L"\\" + dirName;
                LOG_INFO(TEXT("TTSCacheManager: Deleting old cache directory %s"), dirPath.c_str());
                
                WIN32_FIND_DATAW subFindData;
                HANDLE hSubFind = FindFirstFileW((dirPath + L"\\*.*").c_str(), &subFindData);
                if (hSubFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (!(subFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            std::wstring filePath = dirPath + L"\\" + subFindData.cFileName;
                            DeleteFileW(filePath.c_str());
                        }
                    } while (FindNextFileW(hSubFind, &subFindData));
                    FindClose(hSubFind);
                }
                
                RemoveDirectoryW(dirPath.c_str());
            }
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
}

void TTSCacheManager::Initialize() {
    const auto& config = ConfigManager::Inst()->GetConfig();
    int daysToKeep = 7;
    if (config.ttsCacheDaysToKeep > 0) {
        daysToKeep = config.ttsCacheDaysToKeep;
    }
    LOG_INFO(TEXT("TTSCacheManager: Initializing with %d days to keep"), daysToKeep);
    CleanupOldCache(daysToKeep);
}