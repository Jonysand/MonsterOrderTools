# 关键词提取模块

## 概述

本模块负责从舰长弹幕中提取关键词，构建用户画像。

## 技术选型

**cppjieba** - 中文分词库

- 头文件-only 形式，无需编译
- 使用 MixSegment 模式（融合 MP 和 HMM 算法）
- 词典文件：jieba.dict.utf8, hmm_model.utf8, stop_words.utf8

## 目录结构

```
MonsterOrderWilds/
├── cppjieba/                      # cppjieba 源码
│   ├── include/
│   │   └── cppjieba/              # 16个头文件
│   │       ├── Jieba.hpp          # 主入口
│   │       ├── DictTrie.hpp
│   │       ├── MPSegment.hpp
│   │       ├── HMMSegment.hpp
│   │       ├── MixSegment.hpp
│   │       └── ... (其他)
│   └── limonp/                    # 依赖库
│       └── include/
│           └── limonp/
│               ├── StringUtil.hpp
│               └── ... (其他)
└── dict/                          # 词典文件
    ├── jieba.dict.utf8            # 最大概率分词词典 (~4MB)
    ├── hmm_model.utf8             # HMM模型 (~200KB)
    └── stop_words.utf8            # 停用词词典
```

## 接口设计

### 初始化

```cpp
#include "cppjieba/Jieba.hpp"

class CaptainCheckInModule {
private:
    cppjieba::Jieba jieba_;  // 分词器
    
    bool InitJieba() {
        std::string dictPath = GetModuleDictPath();
        std::string jiebaDict = dictPath + "/jieba.dict.utf8";
        std::string hmmModel = dictPath + "/hmm_model.utf8";
        std::string userDict = "";  // 暂不启用
        std::string stopWords = dictPath + "/stop_words.utf8";
        
        jieba_ = cppjieba::Jieba(jiebaDict, hmmModel, userDict, stopWords);
        return true;
    }
};
```

### 关键词提取

```cpp
void ExtractKeywords(UserProfile& profile, const std::string& content) {
    std::vector<std::string> words;
    jieba_.Cut(content, words, true);  // MixSegment 模式
    
    for (const auto& word : words) {
        if (ShouldKeepWord(word)) {
            UpdateKeywordFrequency(profile, word);
        }
    }
}

bool ShouldKeepWord(const std::string& word) {
    if (word.length() < 2) return false;      // 单字过滤
    if (IsStopWord(word)) return false;         // 停用词过滤
    if (IsPunctuation(word)) return false;       // 标点过滤
    return true;
}
```

### 停用词示例

```cpp
static const std::set<std::string> STOP_WORDS = {
    "的", "了", "在", "是", "我", "你", "他", "她", "它",
    "这", "那", "都", "和", "与", "或", "一", "一下",
    "吗", "呢", "吧", "啊", "哦", "嗯", "哈哈", "嘿嘿",
    "可以", "什么", "怎么", "为什么", "有没有", "但是",
    "然后", "所以", "因为", "如果", "虽然", "但是"
};
```

## 防刷屏机制

### 条件检查

```cpp
bool ShouldLearn(const UserProfile& profile, const CaptainDanmuEvent& event) {
    int64_t now = GetCurrentTimestamp();
    
    // 1. 时间窗口检查：距上次发言 < 5秒，跳过
    if (now - profile.lastDanmuTimestamp < 5000) {
        return false;
    }
    
    // 2. 内容去重检查：连续3条相同内容，跳过
    if (!lastContent_.empty() && event.content == lastContent_) {
        sameContentCount_++;
        if (sameContentCount_ >= 3) {
            return false;
        }
    } else {
        sameContentCount_ = 0;
        lastContent_ = event.content;
    }
    
    return true;
}
```

### 计数器更新

```cpp
void UpdateLearnStats(UserProfile& profile, const CaptainDanmuEvent& event) {
    profile.lastDanmuTimestamp = event.serverTimestamp;
}
```

## 数据更新

### 关键词频率更新

```cpp
void UpdateKeywordFrequency(UserProfile& profile, const std::string& word) {
    auto it = std::find_if(profile.keywords.begin(), profile.keywords.end(),
        [&word](const KeywordRecord& r) { return r.word == word; });
    
    if (it != profile.keywords.end()) {
        it->frequency++;
        it->lastSeenTimestamp = GetCurrentTimestamp();
    } else {
        if (profile.keywords.size() >= MAX_KEYWORDS_COUNT) {
            // 移除最低频关键词
            auto minIt = std::min_element(profile.keywords.begin(), profile.keywords.end(),
                [](const KeywordRecord& a, const KeywordRecord& b) { 
                    return a.frequency < b.frequency; 
                });
            profile.keywords.erase(minIt);
        }
        profile.keywords.push_back({word, 1, GetCurrentTimestamp()});
    }
    
    // 按频率排序
    std::sort(profile.keywords.begin(), profile.keywords.end(),
        [](const KeywordRecord& a, const KeywordRecord& b) { 
            return a.frequency > b.frequency; 
        });
}
```

### 弹幕历史更新

```cpp
void UpdateDanmuHistory(UserProfile& profile, const CaptainDanmuEvent& event) {
    profile.danmuHistory.push_back({event.serverTimestamp, event.content});
    
    if (profile.danmuHistory.size() > MAX_DANMU_HISTORY_SIZE) {
        profile.danmuHistory.erase(profile.danmuHistory.begin());
    }
}
```

## 常量定义

```cpp
constexpr size_t MAX_KEYWORDS_COUNT = 50;
constexpr size_t MAX_DANMU_HISTORY_SIZE = 100;
constexpr int32_t MIN_WORD_LENGTH = 2;
constexpr int64_t LEARN_TIME_WINDOW_MS = 5000;
constexpr int32_t MAX_SAME_CONTENT_SKIP = 3;
```

## 文件清单

| 文件 | 职责 |
|------|------|
| `cppjieba/include/cppjieba/*.hpp` | cppjieba 源码（16个头文件） |
| `cppjieba/limonp/include/limonp/*.hpp` | limonp 依赖库头文件 |
| `dict/jieba.dict.utf8` | 最大概率分词词典 |
| `dict/hmm_model.utf8` | HMM 模型 |
| `dict/stop_words.utf8` | 停用词词典 |

## 安装包配置

词典文件需要在安装包中部署到程序运行目录：

| 安装包文件 | 目标路径 |
|------------|----------|
| `dict/jieba.dict.utf8` | `{AppRoot}/dict/jieba.dict.utf8` |
| `dict/hmm_model.utf8` | `{AppRoot}/dict/hmm_model.utf8` |
| `dict/stop_words.utf8` | `{AppRoot}/dict/stop_words.utf8` |
| `dict/弹幕习惯词黑白名单配置.txt` | `{AppRoot}/dict/弹幕习惯词黑白名单配置.txt` |

**注意**：
- 词典文件约 4.2MB，需要在安装包配置中声明
- `弹幕习惯词黑白名单配置.txt` 是配置说明文档
- 运行时会动态读取词典路径，无需硬编码

## 初始化崩溃问题修复记录 (2026-04-13)

**问题描述**：程序在 Release 版本启动时崩溃，异常代码 `0xE06D7363`（C++ EH）和 `0xE0434352`（CLR std::terminate），调用栈显示 clrjit.dll 在 JIT 编译 cppjieba 的 `LoadUserDict` 时崩溃。

**根本原因**：
1. `GetModuleFileNameA` 在某些 Windows 环境（尤其是开启了"Beta: Use Unicode UTF-8"语言选项的系统）下返回的路径编码不是预期的 ANSI/GBK
2. limonp 的 `XCHECK` 宏在 Release 模式下调用 `abort()`，导致 `std::terminate`

**修复方案**：

| 文件 | 修改内容 |
|------|---------|
| `CaptainCheckInModule.cpp` | `GetModuleDictPath()` 改用 `GetModuleFileNameW` + UTF-8 转换 |
| `CaptainCheckInModule.cpp` | 添加 `CreateJiebaContextSafe()` 包装函数，捕获初始化异常 |
| `CaptainCheckInModule.cpp` | `userDict` 参数从空字符串改为 `user.dict.utf8` 文件路径 |
| `limonp/Logging.hpp` | `XCHECK` 的 FATAL 分支改为抛出 `std::runtime_error` 异常而非 `abort()` |
| `main.cpp` | 添加 Global VEH handler 捕获未处理异常 |

**修复效果**：cppjieba 初始化失败时，程序继续运行（关键词提取功能被禁用），其他功能正常。

## MiMo TTS 风格标签过滤 (2026-04-19)

**问题描述**：当使用 MiMo TTS 引擎时，弹幕中的 `#标签#` 格式会被转换为 `<style>标签</style>`，但同时这些标签词会被 `ExtractKeywords` 学习为观众习惯词，导致观众画像不准确。

**根本原因**：
- `XiaomiTTSProvider::HashtagToStyle` 将 `#标签#` 转换为 `<style>标签</style>`
- `ExtractKeywords` 对弹幕内容进行分词学习时，没有排除 TTS 风格标签格式

**修复方案**：

| 文件 | 修改内容 |
|------|---------|
| `CaptainCheckInModule.cpp` | `ExtractKeywords()` 在 MiMo TTS 引擎下，先用正则 `#[^#]+#` 过滤掉 `#标签#` 格式，再进行关键词提取 |

**修复效果**：当使用 MiMo TTS 引擎时，`#标签#` 格式的内容不会被学习为观众习惯词，保持观众画像准确性。
