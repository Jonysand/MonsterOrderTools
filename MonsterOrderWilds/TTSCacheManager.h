#pragma once
#include "framework.h"
#include <string>
#include <vector>

class TTSCacheManager {
    DECLARE_SINGLETON(TTSCacheManager)

public:
    void Initialize();
    std::wstring GetTodayCacheDir() const;
    std::string GetContentPrefix(const std::string& text) const;
    bool SaveCachedAudio(const std::string& text, const std::vector<uint8_t>& audioData);
    void CleanupOldCache(int daysToKeep = 7);

private:
    std::wstring GetCacheBaseDir() const;
    std::string GetTodayDateStr() const;
};