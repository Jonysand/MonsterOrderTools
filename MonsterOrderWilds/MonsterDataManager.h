#pragma once
#include "framework.h"
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <functional>
#include <codecvt>
#include <locale>

// 单个怪物数据
struct OneMonsterData
{
    std::string iconUrl = "";
    std::vector<std::string> nicknames;
    int defaultTemperedLevel = 0;
};

// 怪物匹配结果
struct MonsterMatchResult
{
    std::string monsterName = "";
    int temperedLevel = 0;

    bool HasMatch() const { return !monsterName.empty(); }
};

// 怪物数据管理器 - 负责JSON解析、正则匹配、图标URL管理
class MonsterDataManager
{
    DECLARE_SINGLETON(MonsterDataManager)

public:
    // 加载怪物数据JSON文件
    bool LoadJsonData(const std::string& configPath = "");

    // 匹配怪物名称（输入文本，返回匹配结果）
    MonsterMatchResult GetMatchedMonsterName(const std::string& inputText) const;

    // 获取怪物图标URL
    std::string GetMatchedMonsterIconUrl(const std::string& monsterName) const;

    // 获取所有怪物名称
    std::vector<std::string> GetAllMonsterNames() const;

    // 获取所有怪物信息（用于批量传输到C#层）
    std::vector<OneMonsterData> GetAllMonsterInfo() const;

    // 检查数据是否已加载
    bool IsLoaded() const { return loaded_; }

private:
    MonsterDataManager() = default;
    ~MonsterDataManager() = default;

    // 原始怪物数据
    std::unordered_map<std::string, OneMonsterData> monsterData_;

    // 编译后的正则表达式模式（使用宽字符以支持UTF-8中文）
    struct CompiledPattern
    {
        std::wregex pattern;
        std::string monsterName;
    };
    std::vector<CompiledPattern> compiledPatterns_;

    // 是否已加载
    bool loaded_ = false;
};
