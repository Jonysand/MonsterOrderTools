#pragma once
#include "framework.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

// 字符串处理模块 - 负责中文字符串处理和名称规范化
class StringProcessor
{
public:
    // UTF-8 字符串转 wstring
    static std::wstring Utf8ToWstring(const std::string& str);

    // wstring 转 UTF-8 字符串
    static std::string WstringToUtf8(const std::wstring& wstr);

    // 名称规范化：去除空格、特殊字符，转小写
    static std::string NormalizeName(const std::string& name);

    // 检查字符串是否包含中文字符
    static bool ContainsChinese(const std::string& str);

    // 去除字符串首尾空白
    static std::string Trim(const std::string& str);

    // 字符串分割
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);

    // 字符串替换
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);

    // 缓存管理
    static void ClearCache();
    static size_t GetCacheSize();

private:
    // 转换缓存（减少重复的UTF-8↔wstring转换）
    static std::unordered_map<std::string, std::wstring> utf8ToWstringCache_;
    static std::unordered_map<std::wstring, std::string> wstringToUtf8Cache_;
    static constexpr size_t MAX_CACHE_SIZE = 1024;
    static std::mutex cacheMutex_;
};
