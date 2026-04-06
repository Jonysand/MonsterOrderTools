#include "ConfigFieldRegistry.h"
#include "ConfigManager.h"
#include "DanmuProcessor.h"
#include "CaptainCheckInModule.h"
#include <cstring>

#define REGISTER_FIELD(name, type, member, fieldType) \
    GetMutableFields().emplace_back(name, fieldType, offsetof(ConfigData, member))

#define REGISTER_FIELD_WITH_CALLBACK(name, type, member, fieldType, callback) \
    GetMutableFields().emplace_back(name, fieldType, offsetof(ConfigData, member), callback)

std::vector<ConfigFieldMeta>& ConfigFieldRegistry::GetMutableFields()
{
    static std::vector<ConfigFieldMeta> fields;
    return fields;
}

void ConfigFieldRegistry::RegisterAll()
{
    auto& fields = GetMutableFields();
    if (!fields.empty()) return;

    REGISTER_FIELD("idCode", std::string, idCode, ConfigFieldType::String);
    REGISTER_FIELD("ttsEngine", std::string, ttsEngine, ConfigFieldType::String);
    REGISTER_FIELD("mimoApiKey", std::string, mimoApiKey, ConfigFieldType::String);
    REGISTER_FIELD("mimoVoice", std::string, mimoVoice, ConfigFieldType::String);
    REGISTER_FIELD("mimoStyle", std::string, mimoStyle, ConfigFieldType::String);

    REGISTER_FIELD_WITH_CALLBACK("onlyMedalOrder", bool, onlyMedalOrder, ConfigFieldType::Bool,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlyMedalOrder(cfg.onlyMedalOrder);
        });

    REGISTER_FIELD("enableVoice", bool, enableVoice, ConfigFieldType::Bool);

    REGISTER_FIELD_WITH_CALLBACK("onlySpeekWearingMedal", bool, onlySpeekWearingMedal, ConfigFieldType::Bool,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlySpeekWearingMedal(cfg.onlySpeekWearingMedal);
        });

    REGISTER_FIELD_WITH_CALLBACK("onlySpeekPaidGift", bool, onlySpeekPaidGift, ConfigFieldType::Bool,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlySpeekPaidGift(cfg.onlySpeekPaidGift);
        });

    REGISTER_FIELD("speechRate", int, speechRate, ConfigFieldType::Int);
    REGISTER_FIELD("speechPitch", int, speechPitch, ConfigFieldType::Int);
    REGISTER_FIELD("speechVolume", int, speechVolume, ConfigFieldType::Int);

    REGISTER_FIELD_WITH_CALLBACK("onlySpeekGuardLevel", int, onlySpeekGuardLevel, ConfigFieldType::Int,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlySpeekGuardLevel(cfg.onlySpeekGuardLevel);
        });

    REGISTER_FIELD("opacity", int, opacity, ConfigFieldType::Int);

    REGISTER_FIELD("mimoSpeed", float, mimoSpeed, ConfigFieldType::Float);

    REGISTER_FIELD("topPosX", double, topPosX, ConfigFieldType::Double);
    REGISTER_FIELD("topPosY", double, topPosY, ConfigFieldType::Double);

    REGISTER_FIELD("defaultMarqueeText", std::string, defaultMarqueeText, ConfigFieldType::String);

    REGISTER_FIELD("ttsCacheDaysToKeep", int, ttsCacheDaysToKeep, ConfigFieldType::Int);

    REGISTER_FIELD_WITH_CALLBACK("enableCaptainCheckinAI", bool, enableCaptainCheckinAI, ConfigFieldType::Bool,
        [](ConfigData& cfg) {
            if (CaptainCheckInModule::Inst()->IsEnabled() != cfg.enableCaptainCheckinAI) {
                CaptainCheckInModule::Inst()->SetEnabled(cfg.enableCaptainCheckinAI);
            }
        });

    REGISTER_FIELD_WITH_CALLBACK("checkinTriggerWords", std::string, checkinTriggerWords, ConfigFieldType::String,
        [](ConfigData& cfg) {
            CaptainCheckInModule::Inst()->SetTriggerWords(cfg.checkinTriggerWords);
        });
}

const ConfigFieldMeta* ConfigFieldRegistry::Find(const char* name)
{
    const auto& fields = AllFields();
    for (const auto& field : fields)
    {
        if (strcmp(field.name, name) == 0)
            return &field;
    }
    return nullptr;
}

const ConfigFieldMeta* ConfigFieldRegistry::FindByOffset(size_t offset)
{
    const auto& fields = AllFields();
    for (const auto& field : fields)
    {
        if (field.offset == offset)
            return &field;
    }
    return nullptr;
}

const std::vector<ConfigFieldMeta>& ConfigFieldRegistry::AllFields()
{
    return GetMutableFields();
}

const char* ConfigFieldRegistry::GetString(const ConfigFieldMeta* meta, const ConfigData& config)
{
    if (!meta || meta->type != ConfigFieldType::String) return "";
    const std::string* ptr = reinterpret_cast<const std::string*>(reinterpret_cast<const char*>(&config) + meta->offset);
    return ptr->c_str();
}

void ConfigFieldRegistry::SetString(const ConfigFieldMeta* meta, ConfigData& config, const char* value)
{
    if (!meta || meta->type != ConfigFieldType::String) return;
    char* base = reinterpret_cast<char*>(&config) + meta->offset;
    std::string* ptr = reinterpret_cast<std::string*>(base);
    *ptr = value;
}

bool ConfigFieldRegistry::GetBool(const ConfigFieldMeta* meta, const ConfigData& config)
{
    if (!meta || meta->type != ConfigFieldType::Bool) return false;
    const char* base = reinterpret_cast<const char*>(&config) + meta->offset;
    const bool* ptr = reinterpret_cast<const bool*>(base);
    return *ptr;
}

void ConfigFieldRegistry::SetBool(const ConfigFieldMeta* meta, ConfigData& config, bool value)
{
    if (!meta || meta->type != ConfigFieldType::Bool) return;
    char* base = reinterpret_cast<char*>(&config) + meta->offset;
    bool* ptr = reinterpret_cast<bool*>(base);
    *ptr = value;
}

int ConfigFieldRegistry::GetInt(const ConfigFieldMeta* meta, const ConfigData& config)
{
    if (!meta || meta->type != ConfigFieldType::Int) return 0;
    const char* base = reinterpret_cast<const char*>(&config) + meta->offset;
    const int* ptr = reinterpret_cast<const int*>(base);
    return *ptr;
}

void ConfigFieldRegistry::SetInt(const ConfigFieldMeta* meta, ConfigData& config, int value)
{
    if (!meta || meta->type != ConfigFieldType::Int) return;
    char* base = reinterpret_cast<char*>(&config) + meta->offset;
    int* ptr = reinterpret_cast<int*>(base);
    *ptr = value;
}

float ConfigFieldRegistry::GetFloat(const ConfigFieldMeta* meta, const ConfigData& config)
{
    if (!meta || meta->type != ConfigFieldType::Float) return 0.0f;
    const char* base = reinterpret_cast<const char*>(&config) + meta->offset;
    const float* ptr = reinterpret_cast<const float*>(base);
    return *ptr;
}

void ConfigFieldRegistry::SetFloat(const ConfigFieldMeta* meta, ConfigData& config, float value)
{
    if (!meta || meta->type != ConfigFieldType::Float) return;
    char* base = reinterpret_cast<char*>(&config) + meta->offset;
    float* ptr = reinterpret_cast<float*>(base);
    *ptr = value;
}

double ConfigFieldRegistry::GetDouble(const ConfigFieldMeta* meta, const ConfigData& config)
{
    if (!meta || meta->type != ConfigFieldType::Double) return 0.0;
    const char* base = reinterpret_cast<const char*>(&config) + meta->offset;
    const double* ptr = reinterpret_cast<const double*>(base);
    return *ptr;
}

void ConfigFieldRegistry::SetDouble(const ConfigFieldMeta* meta, ConfigData& config, double value)
{
    if (!meta || meta->type != ConfigFieldType::Double) return;
    char* base = reinterpret_cast<char*>(&config) + meta->offset;
    double* ptr = reinterpret_cast<double*>(base);
    *ptr = value;
}

void ConfigFieldRegistry::InvokeOnChanged(const ConfigFieldMeta* meta, ConfigData& config)
{
    if (meta && meta->onChanged)
    {
        meta->onChanged(config);
    }
}
