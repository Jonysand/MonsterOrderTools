# StringProcessor - 字符串处理

## 概述

中文字符串处理工具类，提供 UTF-8 与 wstring 转换、名称规范化等功能。

## 核心 API

```cpp
class StringProcessor {
    static std::wstring Utf8ToWstring(const std::string& str);
    static std::string WstringToUtf8(const std::wstring& wstr);
    static std::string NormalizeName(const std::string& name);
    static bool ContainsChinese(const std::string& str);
    static std::string Trim(const std::string& str);
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);
    static void ClearCache();
    static size_t GetCacheSize();
};
```

## 缓存机制

- `utf8ToWstringCache_`: UTF-8 → wstring 缓存
- `wstringToUtf8Cache_`: wstring → UTF-8 缓存
- `MAX_CACHE_SIZE`: 1024 条目上限
- 自动缓存转换结果减少重复分配

## 名称规范化

`NormalizeName` 处理：
1. 去除首尾空白
2. 转小写
3. 去除特殊字符

## 文件结构

- `StringProcessor.h`: 接口声明
- `StringProcessor.cpp`: 实现

## 依赖

- Windows API: `MultiByteToWideChar`, `WideCharToMultiByte`
