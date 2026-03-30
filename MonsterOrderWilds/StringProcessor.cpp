#include "framework.h"
#include "StringProcessor.h"
#include <algorithm>
#include <codecvt>
#include <locale>
#include <sstream>

// 静态缓存成员
std::unordered_map<std::string, std::wstring> StringProcessor::utf8ToWstringCache_;
std::unordered_map<std::wstring, std::string> StringProcessor::wstringToUtf8Cache_;

std::wstring StringProcessor::Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return std::wstring();

    // 检查缓存
    auto it = utf8ToWstringCache_.find(str);
    if (it != utf8ToWstringCache_.end())
        return it->second;

    int wideLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (wideLength == 0) return std::wstring();

    std::wstring result(wideLength - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], wideLength);

    // 存入缓存
    if (utf8ToWstringCache_.size() < MAX_CACHE_SIZE)
        utf8ToWstringCache_[str] = result;

    return result;
}

std::string StringProcessor::WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    // 检查缓存
    auto it = wstringToUtf8Cache_.find(wstr);
    if (it != wstringToUtf8Cache_.end())
        return it->second;

    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length == 0) return std::string();

    std::string result(utf8Length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], utf8Length, nullptr, nullptr);

    // 存入缓存
    if (wstringToUtf8Cache_.size() < MAX_CACHE_SIZE)
        wstringToUtf8Cache_[wstr] = result;

    return result;
}

std::string StringProcessor::NormalizeName(const std::string& name)
{
    if (name.empty()) return name;

    std::wstring wide = Utf8ToWstring(name);

    // 转小写
    std::transform(wide.begin(), wide.end(), wide.begin(), ::towlower);

    // 去除空白
    std::wstring trimmed;
    for (wchar_t c : wide)
    {
        if (!iswspace(c))
            trimmed += c;
    }

    return WstringToUtf8(trimmed);
}

bool StringProcessor::ContainsChinese(const std::string& str)
{
    std::wstring wide = Utf8ToWstring(str);
    for (wchar_t c : wide)
    {
        if (c >= 0x4E00 && c <= 0x9FFF)  // CJK统一汉字
            return true;
    }
    return false;
}

std::string StringProcessor::Trim(const std::string& str)
{
    if (str.empty()) return str;

    std::wstring wide = Utf8ToWstring(str);

    // 去除首部空白
    size_t start = 0;
    while (start < wide.length() && iswspace(wide[start]))
        start++;

    // 去除尾部空白
    size_t end = wide.length();
    while (end > start && iswspace(wide[end - 1]))
        end--;

    if (start >= end) return std::string();

    return WstringToUtf8(wide.substr(start, end - start));
}

std::vector<std::string> StringProcessor::Split(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    if (str.empty()) return result;

    size_t pos = 0;
    size_t found;
    while ((found = str.find(delimiter, pos)) != std::string::npos)
    {
        result.push_back(str.substr(pos, found - pos));
        pos = found + delimiter.length();
    }
    result.push_back(str.substr(pos));
    return result;
}

std::string StringProcessor::Replace(const std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty()) return str;

    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos)
    {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

void StringProcessor::ClearCache()
{
    utf8ToWstringCache_.clear();
    wstringToUtf8Cache_.clear();
}

size_t StringProcessor::GetCacheSize()
{
    return utf8ToWstringCache_.size() + wstringToUtf8Cache_.size();
}
