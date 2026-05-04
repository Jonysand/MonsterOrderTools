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
                buffer[min(bufferSize, (DWORD)(sizeof(buffer) - 1))] = '\0';
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
    std::lock_guard<Lock> lock(lock_);
    try
    {
        std::string path = GetConfigPath();
        std::ifstream file(path);
        
        bool configLoaded = false;
        if (file.is_open())
        {
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
            configLoaded = true;

            // 基本配置（idCode 从注册表读取，不从 JSON）
            try { if (j.contains("ONLY_MEDAL_ORDER")) config_.onlyMedalOrder = j["ONLY_MEDAL_ORDER"].get<bool>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ONLY_MEDAL_ORDER wrong type, using default: %s"), e.what()); }
            try { if (j.contains("ENABLE_VOICE")) config_.enableVoice = j["ENABLE_VOICE"].get<bool>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ENABLE_VOICE wrong type, using default: %s"), e.what()); }
            try { if (j.contains("SPEECH_RATE")) config_.speechRate = j["SPEECH_RATE"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: SPEECH_RATE wrong type, using default: %s"), e.what()); }
            try { if (j.contains("SPEECH_PITCH")) config_.speechPitch = j["SPEECH_PITCH"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: SPEECH_PITCH wrong type, using default: %s"), e.what()); }
            try { if (j.contains("SPEECH_VOLUME")) config_.speechVolume = j["SPEECH_VOLUME"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: SPEECH_VOLUME wrong type, using default: %s"), e.what()); }
            try { if (j.contains("ONLY_SPEEK_WEARING_MEDAL")) config_.onlySpeekWearingMedal = j["ONLY_SPEEK_WEARING_MEDAL"].get<bool>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ONLY_SPEEK_WEARING_MEDAL wrong type, using default: %s"), e.what()); }
            try { if (j.contains("ONLY_SPEEK_GUARD_LEVEL")) config_.onlySpeekGuardLevel = j["ONLY_SPEEK_GUARD_LEVEL"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ONLY_SPEEK_GUARD_LEVEL wrong type, using default: %s"), e.what()); }
            try { if (j.contains("ONLY_SPEEK_PAID_GIFT")) config_.onlySpeekPaidGift = j["ONLY_SPEEK_PAID_GIFT"].get<bool>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ONLY_SPEEK_PAID_GIFT wrong type, using default: %s"), e.what()); }
            try { if (j.contains("OPACITY")) config_.opacity = j["OPACITY"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: OPACITY wrong type, using default: %s"), e.what()); }
            try { if (j.contains("PENETRATING_MODE_OPACITY")) config_.penetratingModeOpacity = j["PENETRATING_MODE_OPACITY"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: PENETRATING_MODE_OPACITY wrong type, using default: %s"), e.what()); }

            // 窗口位置
            try {
                if (j.contains("TopPos"))
                {
                    auto& pos = j["TopPos"];
                    if (pos.contains("X")) config_.topPosX = pos["X"].get<double>();
                    if (pos.contains("Y")) config_.topPosY = pos["Y"].get<double>();
                }
            }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: TopPos wrong type, using default: %s"), e.what()); }

            // 跑马灯默认文本
            try { if (j.contains("DEFAULT_MARQUEE_TEXT")) config_.defaultMarqueeText = j["DEFAULT_MARQUEE_TEXT"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: DEFAULT_MARQUEE_TEXT wrong type, using default: %s"), e.what()); }

            // MiMo TTS 配置
            try { if (j.contains("TTS_ENGINE")) config_.ttsEngine = j["TTS_ENGINE"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: TTS_ENGINE wrong type, using default: %s"), e.what()); }
            try { if (j.contains("MIMO_API_KEY")) config_.mimoApiKey = j["MIMO_API_KEY"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MIMO_API_KEY wrong type, using default: %s"), e.what()); }
            try { if (j.contains("MIMO_VOICE")) config_.mimoVoice = j["MIMO_VOICE"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MIMO_VOICE wrong type, using default: %s"), e.what()); }
            try { if (j.contains("MIMO_STYLE")) config_.mimoStyle = j["MIMO_STYLE"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MIMO_STYLE wrong type, using default: %s"), e.what()); }
            try { if (j.contains("MIMO_AUDIO_FORMAT")) config_.mimoAudioFormat = j["MIMO_AUDIO_FORMAT"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MIMO_AUDIO_FORMAT wrong type, using default: %s"), e.what()); }

            // MiniMax TTS 配置
            try { if (j.contains("MINIMAX_VOICE_ID")) config_.minimaxVoiceId = j["MINIMAX_VOICE_ID"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MINIMAX_VOICE_ID wrong type, using default: %s"), e.what()); }
            try { if (j.contains("MINIMAX_SPEED")) config_.minimaxSpeed = j["MINIMAX_SPEED"].get<float>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: MINIMAX_SPEED wrong type, using default: %s"), e.what()); }

            try { if (j.contains("TTS_CACHE_DAYS_TO_KEEP")) config_.ttsCacheDaysToKeep = j["TTS_CACHE_DAYS_TO_KEEP"].get<int>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: TTS_CACHE_DAYS_TO_KEEP wrong type, using default: %s"), e.what()); }

            // 舰长打卡AI配置
            try { if (j.contains("ENABLE_CAPTAIN_CHECKIN_AI")) config_.enableCaptainCheckinAI = j["ENABLE_CAPTAIN_CHECKIN_AI"].get<bool>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: ENABLE_CAPTAIN_CHECKIN_AI wrong type, using default: %s"), e.what()); }
            try { if (j.contains("CHECKIN_TRIGGER_WORDS")) config_.checkinTriggerWords = j["CHECKIN_TRIGGER_WORDS"].get<std::string>(); }
            catch (const std::exception& e) { LOG_DEBUG(TEXT("ConfigManager: CHECKIN_TRIGGER_WORDS wrong type, using default: %s"), e.what()); }
        }
        else
        {
            LOG_DEBUG(TEXT("ConfigManager: Config file not found, using defaults: %s"), path.c_str());
        }

        // 从注册表读取 idCode（无论配置文件是否存在都读取）
        config_.idCode = ReadIdCodeFromRegistry();

        dirty_ = false;
        return configLoaded;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("ConfigManager: LoadConfig failed: %s"), e.what());
        // 即使加载失败，也尝试从注册表读取 idCode
        config_.idCode = ReadIdCodeFromRegistry();
        return false;
    }
}

bool ConfigManager::SaveConfig(bool force)
{
    std::lock_guard<Lock> lock(lock_);
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
        j["PENETRATING_MODE_OPACITY"] = config_.penetratingModeOpacity;

        // 窗口位置
        j["TopPos"] = { {"X", config_.topPosX}, {"Y", config_.topPosY} };

        // MiMo TTS 配置
        j["TTS_ENGINE"] = config_.ttsEngine;
        j["MIMO_API_KEY"] = config_.mimoApiKey;
        j["MIMO_VOICE"] = config_.mimoVoice;
        j["MIMO_STYLE"] = config_.mimoStyle;
        j["MIMO_AUDIO_FORMAT"] = config_.mimoAudioFormat;

        // MiniMax TTS 配置
        j["MINIMAX_VOICE_ID"] = config_.minimaxVoiceId;
        j["MINIMAX_SPEED"] = config_.minimaxSpeed;

        j["TTS_CACHE_DAYS_TO_KEEP"] = config_.ttsCacheDaysToKeep;

        // 舰长打卡AI配置
        j["ENABLE_CAPTAIN_CHECKIN_AI"] = config_.enableCaptainCheckinAI;
        j["CHECKIN_TRIGGER_WORDS"] = config_.checkinTriggerWords;

        // 跑马灯默认文本
        j["DEFAULT_MARQUEE_TEXT"] = config_.defaultMarqueeText;

        std::string path = GetConfigPath();
        std::string tempPath = path + ".tmp";
        std::ofstream file(tempPath);
        if (!file.is_open())
        {
            LOG_ERROR(TEXT("ConfigManager: Cannot open temp config file for writing: %s"), tempPath.c_str());
            return false;
        }

        file << j.dump(4);
        file.close();

        if (!file.good())
        {
            LOG_ERROR(TEXT("ConfigManager: Write to temp config file failed, not renaming: %s"), tempPath.c_str());
            std::filesystem::remove(tempPath);
            return false;
        }

        std::filesystem::rename(tempPath, path);

        // 将 idCode 写入注册表
        if (!WriteIdCodeToRegistry(config_.idCode))
        {
            LOG_ERROR(TEXT("ConfigManager: Failed to write idCode to registry"));
            dirty_ = true;
            return false;
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

ConfigData ConfigManager::GetConfig() const
{
    std::lock_guard<Lock> lock(lock_);
    return config_;
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
    lock_.lock();
    bool changed = config_.speechPitch != value;
    if (changed) { config_.speechPitch = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
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

void ConfigManager::SetMimoAudioFormat(const std::string& value)
{
    lock_.lock();
    bool changed = config_.mimoAudioFormat != value;
    if (changed) { config_.mimoAudioFormat = value; dirty_ = true; }
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

void ConfigManager::SetTtsCacheDaysToKeep(int value)
{
    lock_.lock();
    bool changed = config_.ttsCacheDaysToKeep != value;
    if (changed) { config_.ttsCacheDaysToKeep = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetEnableCaptainCheckinAI(bool value)
{
    lock_.lock();
    bool changed = config_.enableCaptainCheckinAI != value;
    if (changed) { config_.enableCaptainCheckinAI = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::SetCheckinTriggerWords(const std::string& value)
{
    lock_.lock();
    bool changed = config_.checkinTriggerWords != value;
    if (changed) { config_.checkinTriggerWords = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}

void ConfigManager::AddConfigChangedListener(const ConfigChangedHandler& handler)
{
    std::lock_guard<std::mutex> lock(listenerLock_);
    configChangedListeners_.push_back(handler);
}

void ConfigManager::NotifyConfigChanged()
{
    ConfigData configCopy;
    {
        std::lock_guard<Lock> lock(lock_);
        configCopy = config_;
    }
    std::vector<ConfigChangedHandler> listenersCopy;
    {
        std::lock_guard<std::mutex> lock(listenerLock_);
        listenersCopy = configChangedListeners_;
    }
    for (const auto& handler : listenersCopy)
    {
        handler(configCopy);
    }
}

void ConfigManager::SetValueByMeta(const ConfigFieldMeta* meta, const void* value)
{
    if (!meta) return;
    lock_.lock();
    char* base = reinterpret_cast<char*>(&config_) + meta->offset;

    switch (meta->type)
    {
    case ConfigFieldType::Bool:
        *reinterpret_cast<bool*>(base) = *static_cast<const bool*>(value);
        break;
    case ConfigFieldType::Int:
        *reinterpret_cast<int*>(base) = *static_cast<const int*>(value);
        break;
    case ConfigFieldType::Float:
        *reinterpret_cast<float*>(base) = *static_cast<const float*>(value);
        break;
    case ConfigFieldType::Double:
        *reinterpret_cast<double*>(base) = *static_cast<const double*>(value);
        break;
    case ConfigFieldType::String:
        *reinterpret_cast<std::string*>(base) = *static_cast<const std::string*>(value);
        break;
    }
    dirty_ = true;
    ConfigData configCopy = config_;
    lock_.unlock();

    ConfigFieldRegistry::InvokeOnChanged(meta, configCopy);
    NotifyConfigChanged();
}
