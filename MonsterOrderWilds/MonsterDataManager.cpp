#include "framework.h"
#include "MonsterDataManager.h"
#include "WriteLog.h"
#include <fstream>
#include <sstream>
#include <filesystem>

DEFINE_SINGLETON(MonsterDataManager)

bool MonsterDataManager::LoadJsonData(const std::string& configPath)
{
    try
    {
        std::string path = configPath;
        if (path.empty())
        {
            path = "MonsterOrderWilds_configs/monster_list.json";
        }

        if (!std::filesystem::exists(path))
        {
            LOG_ERROR(TEXT("MonsterDataManager: Cannot find monster list: %s"), path.c_str());
            return false;
        }

        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_ERROR(TEXT("MonsterDataManager: Cannot open file: %s"), path.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        if (content.size() >= 3 &&
            (unsigned char)content[0] == 0xEF &&
            (unsigned char)content[1] == 0xBB &&
            (unsigned char)content[2] == 0xBF)
        {
            content = content.substr(3);
        }

        json j = json::parse(content);
        monsterData_.clear();
        compiledPatterns_.clear();

        for (auto& [key, value] : j.items())
        {
            OneMonsterData data;
            if (value.contains("图标地址"))
                data.iconUrl = value["图标地址"].get<std::string>();
            if (value.contains("别称"))
                data.nicknames = value["别称"].get<std::vector<std::string>>();
            if (value.contains("默认历战等级"))
                data.defaultTemperedLevel = value["默认历战等级"].get<int>();

            monsterData_[key] = data;

            for (const auto& nickname : data.nicknames)
            {
                try
                {
                    std::wstring wnickname = Utf8ToWstring(nickname);
                    std::wstring wkey = Utf8ToWstring(key);
                    CompiledPattern cp;
                    cp.pattern = std::wregex(wnickname);
                    cp.monsterName = key;
                    compiledPatterns_.push_back(cp);
                    LOGW_DEBUG(L"MonsterDataManager: Compiled pattern '%s' for monster '%s'", wnickname.c_str(), wkey.c_str());
                }
                catch (const std::regex_error& e)
                {
                    LOG_ERROR(TEXT("MonsterDataManager: Regex compile failed for '%s': %s"), nickname.c_str(), e.what());
                }
            }
        }

        LOGW_DEBUG(L"MonsterDataManager: Loaded %d monsters with %d patterns", (int)monsterData_.size(), (int)compiledPatterns_.size());
        loaded_ = true;
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("MonsterDataManager: LoadJsonData failed: %s"), e.what());
        return false;
    }
}

MonsterMatchResult MonsterDataManager::GetMatchedMonsterName(const std::string& inputText) const
{
    MonsterMatchResult result;
    if (!loaded_ || compiledPatterns_.empty())
        return result;

    std::wstring winput = Utf8ToWstring(inputText);

    for (const auto& cp : compiledPatterns_)
    {
        std::wsmatch match;
        if (std::regex_search(winput, match, cp.pattern))
        {
            result.monsterName = cp.monsterName;
            auto it = monsterData_.find(cp.monsterName);
            if (it != monsterData_.end())
            {
                result.temperedLevel = it->second.defaultTemperedLevel;
            }
            return result;
        }
    }
    return result;
}

std::string MonsterDataManager::GetMatchedMonsterIconUrl(const std::string& monsterName) const
{
    auto it = monsterData_.find(monsterName);
    if (it != monsterData_.end())
        return it->second.iconUrl;
    return "";
}

std::vector<std::string> MonsterDataManager::GetAllMonsterNames() const
{
    std::vector<std::string> names;
    names.reserve(monsterData_.size());
    for (const auto& [key, value] : monsterData_)
    {
        names.push_back(key);
    }
    return names;
}

std::vector<OneMonsterData> MonsterDataManager::GetAllMonsterInfo() const
{
    std::vector<OneMonsterData> result;
    result.reserve(monsterData_.size());
    for (const auto& [key, value] : monsterData_)
    {
        result.push_back(value);
    }
    return result;
}