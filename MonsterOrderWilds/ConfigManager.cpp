#include "framework.h"
#include "ConfigManager.h"
#include "WriteLog.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <Windows.h>
#include <WinReg.h>

DEFINE_SINGLETON(ConfigManager)

namespace
{
    const char* REG_SUBKEY = "Software\\MonsterOrderWilds";
    const char* REG_VALUE_NAME = "IdCode";

    std::string ReadIdCodeFromRegistry()
    {
        std::string result;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char buffer[256];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, REG_VALUE_NAME, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
            {
                result = buffer;
            }
            RegCloseKey(hKey);
        }
        return result;
    }

    bool WriteIdCodeToRegistry(const std::string& idCode)
    {
        HKEY hKey;
        DWORD disp;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) == ERROR_SUCCESS)
        {
            RegSetValueExA(hKey, REG_VALUE_NAME, 0, REG_SZ, (const BYTE*)idCode.c_str(), idCode.length() + 1);
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
}

std::string ConfigManager::GetConfigDirectory() const
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeFullPath(exePath);
    std::filesystem::path exeDir = exeFullPath.parent_path();
    return (exeDir / "MonsterOrderWilds_configs").string();
}

std::string ConfigManager::GetConfigPath() const
{
    return GetConfigDirectory() + "/MainConfig.cfg";
}

bool ConfigManager::LoadConfig()
{
    try
    {
        std::string path = GetConfigPath();
        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR(TEXT("ConfigManager: Cannot open config file: %s"), path.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // 跳过BOM（如果存在）
        if (content.size() >= 3 &&
            (unsigned char)content[0] == 0xEF &&
            (unsigned char)content[1] == 0xBB &&
            (unsigned char)content[2] == 0xBF)
        {
            content = content.substr(3);
        }

        json j = json::parse(content);

        // 基本配置（idCode 从注册表读取，不从 JSON）
        if (j.contains("ONLY_MEDAL_ORDER")) config_.onlyMedalOrder = j["ONLY_MEDAL_ORDER"].get<bool>();
        if (j.contains("ENABLE_VOICE")) config_.enableVoice = j["ENABLE_VOICE"].get<bool>();
        if (j.contains("SPEECH_RATE")) config_.speechRate = j["SPEECH_RATE"].get<int>();
        if (j.contains("SPEECH_PITCH")) config_.speechPitch = j["SPEECH_PITCH"].get<int>();
        if (j.contains("SPEECH_VOLUME")) config_.speechVolume = j["SPEECH_VOLUME"].get<int>();
        if (j.contains("ONLY_SPEEK_WEARING_MEDAL")) config_.onlySpeekWearingMedal = j["ONLY_SPEEK_WEARING_MEDAL"].get<bool>();
        if (j.contains("ONLY_SPEEK_GUARD_LEVEL")) config_.onlySpeekGuardLevel = j["ONLY_SPEEK_GUARD_LEVEL"].get<int>();
        if (j.contains("ONLY_SPEEK_PAID_GIFT")) config_.onlySpeekPaidGift = j["ONLY_SPEEK_PAID_GIFT"].get<bool>();
        if (j.contains("OPACITY")) config_.opacity = j["OPACITY"].get<int>();

        // 窗口位置
        if (j.contains("TopPos"))
        {
            auto& pos = j["TopPos"];
            if (pos.contains("X")) config_.topPosX = pos["X"].get<double>();
            if (pos.contains("Y")) config_.topPosY = pos["Y"].get<double>();
        }

        // 跑马灯默认文本
        if (j.contains("DEFAULT_MARQUEE_TEXT")) config_.defaultMarqueeText = j["DEFAULT_MARQUEE_TEXT"].get<std::string>();

        // MiMo TTS 配置
        if (j.contains("TTS_ENGINE")) config_.ttsEngine = j["TTS_ENGINE"].get<std::string>();
        if (j.contains("MIMO_API_KEY")) config_.mimoApiKey = j["MIMO_API_KEY"].get<std::string>();
        if (j.contains("MIMO_VOICE")) config_.mimoVoice = j["MIMO_VOICE"].get<std::string>();
        if (j.contains("MIMO_STYLE")) config_.mimoStyle = j["MIMO_STYLE"].get<std::string>();
        if (j.contains("MIMO_DIALECT")) config_.mimoDialect = j["MIMO_DIALECT"].get<std::string>();
        if (j.contains("MIMO_ROLE")) config_.mimoRole = j["MIMO_ROLE"].get<std::string>();
        if (j.contains("MIMO_AUDIO_FORMAT")) config_.mimoAudioFormat = j["MIMO_AUDIO_FORMAT"].get<std::string>();
        if (j.contains("MIMO_SPEED")) config_.mimoSpeed = j["MIMO_SPEED"].get<float>();

        // 从注册表读取 idCode
        config_.idCode = ReadIdCodeFromRegistry();

        dirty_ = false;
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("ConfigManager: LoadConfig failed: %s"), e.what());
        return false;
    }
}

bool ConfigManager::SaveConfig(bool force)
{
    if (!force && !dirty_)
        return true;

    try
    {
        std::string dir = GetConfigDirectory();
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        json j;
        // 基本配置（idCode 写入注册表，不写入 JSON）
        j["ONLY_MEDAL_ORDER"] = config_.onlyMedalOrder;
        j["ENABLE_VOICE"] = config_.enableVoice;
        j["SPEECH_RATE"] = config_.speechRate;
        j["SPEECH_PITCH"] = config_.speechPitch;
        j["SPEECH_VOLUME"] = config_.speechVolume;
        j["ONLY_SPEEK_WEARING_MEDAL"] = config_.onlySpeekWearingMedal;
        j["ONLY_SPEEK_GUARD_LEVEL"] = config_.onlySpeekGuardLevel;
        j["ONLY_SPEEK_PAID_GIFT"] = config_.onlySpeekPaidGift;
        j["OPACITY"] = config_.opacity;

        // 窗口位置
        j["TopPos"] = { {"X", config_.topPosX}, {"Y", config_.topPosY} };

        // MiMo TTS 配置
        j["TTS_ENGINE"] = config_.ttsEngine;
        j["MIMO_API_KEY"] = config_.mimoApiKey;
        j["MIMO_VOICE"] = config_.mimoVoice;
        j["MIMO_STYLE"] = config_.mimoStyle;
        j["MIMO_DIALECT"] = config_.mimoDialect;
        j["MIMO_ROLE"] = config_.mimoRole;
        j["MIMO_AUDIO_FORMAT"] = config_.mimoAudioFormat;
        j["MIMO_SPEED"] = config_.mimoSpeed;

        // 跑马灯默认文本
        j["DEFAULT_MARQUEE_TEXT"] = config_.defaultMarqueeText;

        std::string path = GetConfigPath();
        std::ofstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR(TEXT("ConfigManager: Cannot open config file for writing: %s"), path.c_str());
            return false;
        }

        file << j.dump(4);
        file.close();

        // 将 idCode 写入注册表
        if (!WriteIdCodeToRegistry(config_.idCode))
        {
            LOG_ERROR(TEXT("ConfigManager: Failed to write idCode to registry"));
        }

        dirty_ = false;
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("ConfigManager: SaveConfig failed: %s"), e.what());
        return false;
    }
}

void ConfigManager::MarkDirty()
{
    dirty_ = true;
}

const ConfigData& ConfigManager::GetConfig() const
{
    lock_.lock();
    cachedConfig_ = config_;  // 拷贝到缓存
    lock_.unlock();
    return cachedConfig_;
}

void ConfigManager::UpdateConfig(const ConfigData& newData)
{
    lock_.lock();
    config_ = newData;
    dirty_ = true;
    lock_.unlock();
    NotifyConfigChanged();
}

void ConfigManager::SetIdCode(const std::string& value)
{
    lock_.lock();
    bool changed = config_.idCode != value;
    if (changed) { config_.idCode = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetOnlyMedalOrder(bool value)
{
    lock_.lock();
    bool changed = config_.onlyMedalOrder != value;
    if (changed) { config_.onlyMedalOrder = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetEnableVoice(bool value)
{
    lock_.lock();
    bool changed = config_.enableVoice != value;
    if (changed) { config_.enableVoice = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetSpeechRate(int value)
{
    lock_.lock();
    bool changed = config_.speechRate != value;
    if (changed) { config_.speechRate = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetSpeechPitch(int value)
{
    if (config_.speechPitch != value) { config_.speechPitch = value; dirty_ = true; NotifyConfigChanged(); }
}

void ConfigManager::SetSpeechVolume(int value)
{
    lock_.lock();
    bool changed = config_.speechVolume != value;
    if (changed) { config_.speechVolume = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetOnlySpeekWearingMedal(bool value)
{
    lock_.lock();
    bool changed = config_.onlySpeekWearingMedal != value;
    if (changed) { config_.onlySpeekWearingMedal = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetOnlySpeekGuardLevel(int value)
{
    lock_.lock();
    bool changed = config_.onlySpeekGuardLevel != value;
    if (changed) { config_.onlySpeekGuardLevel = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetOnlySpeekPaidGift(bool value)
{
    lock_.lock();
    bool changed = config_.onlySpeekPaidGift != value;
    if (changed) { config_.onlySpeekPaidGift = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetOpacity(int value)
{
    lock_.lock();
    bool changed = config_.opacity != value;
    if (changed) { config_.opacity = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetTtsEngine(const std::string& value)
{
    lock_.lock();
    bool changed = config_.ttsEngine != value;
    if (changed) { config_.ttsEngine = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoApiKey(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoApiKey != value;
    if (changed) { config_.mimoApiKey = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoVoice(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoVoice != value;
    if (changed) { config_.mimoVoice = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoStyle(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoStyle != value;
    if (changed) { config_.mimoStyle = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoDialect(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoDialect != value;
    if (changed) { config_.mimoDialect = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoRole(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoRole != value;
    if (changed) { config_.mimoRole = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoAudioFormat(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoAudioFormat != value;
    if (changed) { config_.mimoAudioFormat = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetMimoSpeed(float value)
{
    lock_.lock();
    bool changed = config_.mimoSpeed != value;
    if (changed) { config_.mimoSpeed = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetWindowPosition(double x, double y)
{
    lock_.lock();
    bool changed = config_.topPosX != x || config_.topPosY != y;
    if (changed)
    {
        config_.topPosX = x;
        config_.topPosY = y;
        dirty_ = true;
    }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::AddConfigChangedListener(const ConfigChangedHandler& handler)
{
    configChangedListeners_.push_back(handler);
}

void ConfigManager::NotifyConfigChanged()
{
    for (const auto& handler : configChangedListeners_)
    {
        handler(config_);
    }
}

void ConfigManager::SetValueByMeta(const ConfigFieldMeta* meta, const void* value)
{
    if (!meta) return;
    lock_.lock();
    char* base = reinterpret_cast<char*>(&config_) + meta->offset;
    char* cachedBase = reinterpret_cast<char*>(&cachedConfig_) + meta->offset;
    
    switch (meta->type)
    {
    case ConfigFieldType::Bool:
        *reinterpret_cast<bool*>(base) = *static_cast<const bool*>(value);
        *reinterpret_cast<bool*>(cachedBase) = *static_cast<const bool*>(value);
        break;
    case ConfigFieldType::Int:
        *reinterpret_cast<int*>(base) = *static_cast<const int*>(value);
        *reinterpret_cast<int*>(cachedBase) = *static_cast<const int*>(value);
        break;
    case ConfigFieldType::Float:
        *reinterpret_cast<float*>(base) = *static_cast<const float*>(value);
        *reinterpret_cast<float*>(cachedBase) = *static_cast<const float*>(value);
        break;
    case ConfigFieldType::Double:
        *reinterpret_cast<double*>(base) = *static_cast<const double*>(value);
        *reinterpret_cast<double*>(cachedBase) = *static_cast<const double*>(value);
        break;
    case ConfigFieldType::String:
        *reinterpret_cast<std::string*>(base) = *static_cast<const std::string*>(value);
        *reinterpret_cast<std::string*>(cachedBase) = *static_cast<const std::string*>(value);
        break;
    }
    dirty_ = true;
    lock_.unlock();
}
