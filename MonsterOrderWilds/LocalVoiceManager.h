#pragma once
#include "framework.h"
#include <string>
#include <vector>
#include <unordered_map>

class LocalVoiceManager {
    DECLARE_SINGLETON(LocalVoiceManager)

public:

    // 匹配弹幕文本，返回 zip 内文件名（空字符串表示不匹配）
    std::string MatchVoice(const std::wstring& msg) const;

    // 从 zip 读取音频文件到内存
    bool LoadVoiceData(const std::string& voiceFile, std::vector<uint8_t>& outData);

private:
    LocalVoiceManager();
    ~LocalVoiceManager();
    LocalVoiceManager(const LocalVoiceManager&) = delete;
    LocalVoiceManager& operator=(const LocalVoiceManager&) = delete;

    std::string zipPath_;
    std::unordered_map<std::wstring, std::string> voiceMap_;
};
