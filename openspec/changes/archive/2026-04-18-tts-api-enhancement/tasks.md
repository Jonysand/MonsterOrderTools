# TTS API Enhancement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** MiniMax API 参数标准化 + Mimo #标签 转 <style> 标签

**Architecture:** 修改 C++ TTS Provider 实现层：MiniMaxTTSProvider::BuildRequestBody 和 XiaomiTTSProvider::BuildRequestBody

**Tech Stack:** ["C++", "nlohmann::json", "WinHTTP", "Regex"]

---

## 1. MiniMax API 参数标准化

**Files:**
- Modify: `MonsterOrderWilds/MiniMaxTTSProvider.cpp:79-98`

**精确执行序列：**

- [ ] **1.1 修改 MiniMaxTTSProvider::BuildRequestBody - voice_setting**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/MiniMaxTTSProvider.cpp`，第 85-91 行
  2. 用 edit 工具修改 voice_setting，添加 `"text_normalization", true`

- [ ] **1.2 修改 MiniMaxTTSProvider::BuildRequestBody - audio_setting**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/MiniMaxTTSProvider.cpp`，第 92-97 行
  2. 用 edit 工具修改 audio_setting 的 sample_rate 为 22050，bitrate 为 64000

## 2. Mimo #标签 转 <style> 标签

**Files:**
- Modify: `MonsterOrderWilds/XiaomiTTSProvider.cpp:80-95`

**精确执行序列：**

- [ ] **2.1 添加 HashtagToStyle 函数**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/XiaomiTTSProvider.cpp`
  2. 在文件末尾（186行后）添加辅助函数：

```cpp
std::string HashtagToStyle(const std::string& input) const {
    std::string result = input;
    std::regex pattern(R"(#([^#]+)#)");
    std::smatch match;
    std::string::const_iterator searchStart(result.cbegin());
    while (std::regex_search(searchStart, result.cend(), match, pattern)) {
        std::string tag = match[1].str();
        std::string replacement = "<style>" + tag + "</style>";
        result.replace(match[0].first, match[0].second, replacement);
        searchStart = result.cbegin() + (replacement.length() - match[0].length() + (searchStart - result.cbegin()));
    }
    return result;
}
```

- [ ] **2.2 修改 XiaomiTTSProvider::BuildRequestBody 调用 HashtagToStyle**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/XiaomiTTSProvider.cpp`，第 80-95 行
  2. 用 edit 工具修改 BuildRequestBody，在处理 request.text 前先调用 HashtagToStyle

## 3. 验证与测试

**精确执行序列：**

- [ ] **3.1 编译验证**

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

- [ ] **3.2 git diff 检查**

  【工具序列】bash
  执行: `git diff MonsterOrderWilds/MiniMaxTTSProvider.cpp MonsterOrderWilds/XiaomiTTSProvider.cpp`
  预期: 显示正确的修改内容
