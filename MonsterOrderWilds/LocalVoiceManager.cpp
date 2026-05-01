#include "framework.h"
#include "LocalVoiceManager.h"
#include "ConfigManager.h"
#include "WriteLog.h"
#include "../external/miniz/miniz.h"
#include <algorithm>
#include <cctype>

DEFINE_SINGLETON(LocalVoiceManager)

namespace {
    std::wstring ToLower(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](wchar_t c) { return std::tolower(c); });
        return result;
    }
}

LocalVoiceManager::LocalVoiceManager() {
    zipPath_ = ConfigManager::Inst()->GetConfigDirectory() + "/local_voices.zip";

    // 初始化弹幕->音频文件映射（zip 内路径包含 manbo/ 前缀）
    voiceMap_[TEXT("曼波")] = "manbo/manbo.mp3";
    voiceMap_[TEXT("曼波曼波")] = "manbo/manbo_3x.mp3";
    voiceMap_[TEXT("duang")] = "manbo/duang.mp3";
    voiceMap_[TEXT("噢耶")] = "manbo/ohyeah.mp3";
    voiceMap_[TEXT("哦耶")] = "manbo/ohyeah.mp3";
    voiceMap_[TEXT("欧耶")] = "manbo/ohyeah.mp3";
    voiceMap_[TEXT("wow")] = "manbo/wow.mp3";

    LOG_DEBUG(TEXT("LocalVoiceManager initialized, zip path: %s"), Utf8ToWstring(zipPath_).c_str());
}

LocalVoiceManager::~LocalVoiceManager() {}

std::string LocalVoiceManager::MatchVoice(const std::wstring& msg) const {
    // 精确匹配（忽略大小写）
    auto it = voiceMap_.find(ToLower(msg));
    if (it != voiceMap_.end()) {
        return it->second;
    }
    return "";
}

bool LocalVoiceManager::LoadVoiceData(const std::string& voiceFile, std::vector<uint8_t>& outData) {
    // 打开 zip 文件
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, zipPath_.c_str(), 0)) {
        LOG_ERROR(TEXT("LocalVoiceManager: Failed to open zip file: %s"), Utf8ToWstring(zipPath_).c_str());
        return false;
    }

    // 查找文件
    int fileIndex = mz_zip_reader_locate_file(&zipArchive, voiceFile.c_str(), nullptr, 0);
    if (fileIndex < 0) {
        LOG_ERROR(TEXT("LocalVoiceManager: File not found in zip: %s"), Utf8ToWstring(voiceFile).c_str());
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    // 获取文件信息
    mz_zip_archive_file_stat fileStat;
    if (!mz_zip_reader_file_stat(&zipArchive, fileIndex, &fileStat)) {
        LOG_ERROR(TEXT("LocalVoiceManager: Failed to get file stat for: %s"), Utf8ToWstring(voiceFile).c_str());
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    // 读取文件内容
    size_t uncompressedSize = (size_t)fileStat.m_uncomp_size;
    outData.resize(uncompressedSize);

    if (!mz_zip_reader_extract_to_mem(&zipArchive, fileIndex, outData.data(), uncompressedSize, 0)) {
        LOG_ERROR(TEXT("LocalVoiceManager: Failed to extract file: %s"), Utf8ToWstring(voiceFile).c_str());
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    mz_zip_reader_end(&zipArchive);
    LOG_DEBUG(TEXT("LocalVoiceManager: Loaded %s, size=%zu bytes"), Utf8ToWstring(voiceFile).c_str(), uncompressedSize);
    return true;
}
