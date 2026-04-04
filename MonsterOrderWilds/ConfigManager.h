#pragma once
#include "framework.h"
#include "EventSystem.h"
#include "ConfigFieldRegistry.h"
#include <functional>

// 配置数据结构（纯数据，无逻辑）
struct ConfigData
{
    // 基本配置
    std::string idCode = "";
    bool onlyMedalOrder = true;
    bool enableVoice = false;
    int speechRate = 0;
    int speechPitch = 0;
    int speechVolume = 0;
    bool onlySpeekWearingMedal = false;
    int onlySpeekGuardLevel = 0;
    bool onlySpeekPaidGift = false;
    int opacity = 100;

    // MiMo TTS 配置
    std::string ttsEngine = "auto";
    std::string mimoApiKey = "";
    std::string mimoVoice = "mimo_default";
    std::string mimoStyle = "";
    std::string mimoDialect = "";
    std::string mimoRole = "";
    std::string mimoAudioFormat = "mp3";
    float mimoSpeed = 1.0f;

    // 窗口位置
    double topPosX = 0.0;
    double topPosY = 0.0;

    // 跑马灯默认文本
    std::string defaultMarqueeText = "";

    // TTS缓存配置
    int ttsCacheDaysToKeep = 7;
};

// 配置管理器 - 负责配置加载、保存和变更通知
class ConfigManager
{
    DECLARE_SINGLETON(ConfigManager)

public:
    // 加载配置文件
    bool LoadConfig();
    // 保存配置文件（force: 是否强制保存，忽略dirty标记）
    bool SaveConfig(bool force = false);
    // 标记配置已修改
    void MarkDirty();
    // 获取当前配置（只读引用）
    const ConfigData& GetConfig() const;
    // 更新配置（批量更新，触发一次变更事件）
    void UpdateConfig(const ConfigData& newData);
    // 更新单个字段
    void SetIdCode(const std::string& value);
    void SetOnlyMedalOrder(bool value);
    void SetEnableVoice(bool value);
    void SetSpeechRate(int value);
    void SetSpeechPitch(int value);
    void SetSpeechVolume(int value);
    void SetOnlySpeekWearingMedal(bool value);
    void SetOnlySpeekGuardLevel(int value);
    void SetOnlySpeekPaidGift(bool value);
    void SetOpacity(int value);
    void SetTtsEngine(const std::string& value);
    void SetMimoApiKey(const std::string& value);
    void SetMimoVoice(const std::string& value);
    void SetMimoStyle(const std::string& value);
    void SetMimoDialect(const std::string& value);
    void SetMimoRole(const std::string& value);
    void SetMimoAudioFormat(const std::string& value);
    void SetMimoSpeed(float value);
    void SetWindowPosition(double x, double y);
    void SetTtsCacheDaysToKeep(int value);

    // 通过 ConfigFieldMeta 设置配置值（用于 DataBridge）
    void SetValueByMeta(const ConfigFieldMeta* meta, const void* value);

    // 配置变更事件
    using ConfigChangedHandler = std::function<void(const ConfigData&)>;
    void AddConfigChangedListener(const ConfigChangedHandler& handler);
private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // 配置文件路径
    std::string GetConfigPath() const;
    std::string GetConfigDirectory() const;

    // 数据
    ConfigData config_;
    bool dirty_ = false;
    std::string configDir_;
    std::string configFile_;

    // 缓存（用于减少锁争用，读操作直接返回缓存）
    mutable ConfigData cachedConfig_;

    // 线程安全
    mutable Lock lock_;

    // 配置变更事件
    std::vector<ConfigChangedHandler> configChangedListeners_;
    void NotifyConfigChanged();
};
