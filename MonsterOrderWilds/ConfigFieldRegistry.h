#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <functional>

struct ConfigData;

enum class ConfigFieldType : int
{
    String = 0,
    Bool = 1,
    Int = 2,
    Float = 3,
    Double = 4
};

using ConfigChangeHandler = std::function<void(ConfigData&)>;

struct ConfigFieldMeta
{
    const char* name;
    ConfigFieldType type;
    size_t offset;
    ConfigChangeHandler onChanged;

    ConfigFieldMeta(const char* n, ConfigFieldType t, size_t o, ConfigChangeHandler cb = nullptr)
        : name(n), type(t), offset(o), onChanged(std::move(cb)) {}
};

class ConfigFieldRegistry
{
public:
    static void RegisterAll();

    static const ConfigFieldMeta* Find(const char* name);
    static const ConfigFieldMeta* FindByOffset(size_t offset);

    static const char* GetString(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetString(const ConfigFieldMeta* meta, ConfigData& config, const char* value);

    static bool GetBool(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetBool(const ConfigFieldMeta* meta, ConfigData& config, bool value);

    static int GetInt(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetInt(const ConfigFieldMeta* meta, ConfigData& config, int value);

    static float GetFloat(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetFloat(const ConfigFieldMeta* meta, ConfigData& config, float value);

    static double GetDouble(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetDouble(const ConfigFieldMeta* meta, ConfigData& config, double value);

    static void InvokeOnChanged(const ConfigFieldMeta* meta, ConfigData& config);

    static const std::vector<ConfigFieldMeta>& AllFields();

private:
    static std::vector<ConfigFieldMeta>& GetMutableFields();
};
