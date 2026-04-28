# Code Review Fixes - Design Document

## 概述

本次代码审查修复了项目中的多线程安全问题和代码结构问题，涉及 14 个文件的修改。

## 问题分类

### 1. 多线程安全问题 (P0)

| 问题 | 文件 | 修复方案 |
|------|------|----------|
| StringProcessor 缓存无锁保护 | StringProcessor.h/cpp | 添加 cacheMutex_ 使用 lock_guard |
| CredentialsManager 全局变量无锁 | CredentialsManager.cpp | 添加 g_mutex 保护全局变量 |
| TextToSpeech 队列数据竞争 | TextToSpeech.h/cpp | WebSocket 回调中加锁，shared_ptr 管理生命周期 |
| ConfigManager SetSpeechPitch 无锁 | ConfigManager.cpp | 添加 lock_guard 保护 |
| TTSManager 锁类型混用 | TextToSpeech.h/cpp | 统一使用 mutex，添加 Internal 函数 |

### 2. 代码结构问题 (P1)

| 问题 | 文件 | 修复方案 |
|------|------|----------|
| ToolsMain if-else 链 | ToolsMain.cs | 重构为字典分发 _configHandlers |
| ConfigWindow 重复 ConfigChanged | ConfigWindow.xaml.cs | 移除连接按钮中的重复调用 |
| DanmuProcessor 过滤缺失 | DanmuProcessor.cpp | 补充 guard_level 和 paid_gift 过滤 |
| PriorityQueueManager 非原子写入 | PriorityQueueManager.cpp | 临时文件 + rename 原子写入 |
| Network HTTP 线程无异常处理 | Network.cpp | 添加 try-catch 包裹线程主体 |

### 3. 代码质量改进 (P2)

| 问题 | 文件 | 修复方案 |
|------|------|----------|
| ProfileManager 跨年计算死代码 | ProfileManager.cpp | 移除 year == lastYear + 2 分支 |
| BliveManager 魔法数字 | BliveManager.h/cpp | 提取为命名常量 |

## 技术细节

### 1. TextToSpeech 线程安全重构

**问题:** WebSocket 回调在不同线程执行，直接访问 `pendingRequests_` 队列存在数据竞争。

**修复:**
```cpp
// 之前：直接 push
void HandleSpeekDm(const std::string& str) {
    auto request = std::make_shared<AsyncTTSRequest>(str, TTS_REQUEST_SPEAK_DM);
    pendingRequests_.push(request);  // 数据竞争！
}

// 之后：加锁保护
void HandleSpeekDm(const std::string& str) {
    auto request = std::make_shared<AsyncTTSRequest>(str, TTS_REQUEST_SPEAK_DM);
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingRequests_.push(request);
}
```

### 2. ToolsMain 配置处理重构

**问题:** 25+ if-else 分支违反开闭原则。

**修复:**
```csharp
// 之前
if (key == "ID_CODE") { _Config.Config.ID_CODE = value; }
else if (key == "VOICE_NAME") { _Config.Config.VOICE_NAME = value; }
// ... 25+ 分支

// 之后
private void InitConfigHandlers()
{
    _configHandlers = new Dictionary<string, Action<string, string>>
    {
        ["ID_CODE"] = (k, v) => _Config.Config.ID_CODE = v,
        ["VOICE_NAME"] = (k, v) => _Config.Config.VOICE_NAME = v,
        // ... 易于扩展
    };
}
```

### 3. PriorityQueueManager 原子写入

**问题:** 直接写入文件可能在写入中途崩溃导致文件损坏。

**修复:**
```cpp
// 之前
std::ofstream file(path);
file << j.dump(4);
file.close();

// 之后
std::string tempPath = path + ".tmp";
std::ofstream tempFile(tempPath);
tempFile << j.dump(4);
tempFile.close();
std::filesystem::rename(tempPath, path);  // 原子操作
```

## 编译验证

✅ MSBuild Release x64 编译通过，0 个错误