# Code Review Fixes - Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修复代码审查发现的多线程安全问题和代码结构问题

**Architecture:** C++/C# 混合项目，涉及 Bilibili 直播助手工具的弹幕处理、TTS语音播报等功能

**Tech Stack:** ["C++", "C#", "Win32", "SQLite3"]

---

## 1. P0 严重问题修复

### 1.1 StringProcessor 缓存线程安全

**Files:**
- Modify: `MonsterOrderWilds/StringProcessor.h`
- Modify: `MonsterOrderWilds/StringProcessor.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 添加 `cacheMutex_` 保护 `utf8ToWstringCache_` 和 `wstringToUtf8Cache_`
- 使用 `lock_guard<mutex>` 确保多线程安全访问

### 1.2 CredentialsManager 全局变量线程安全

**Files:**
- Modify: `MonsterOrderWilds/CredentialsManager.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 添加 `g_mutex` 保护全局变量 `g_encryptedKey_` 和 `g_encryptedIV_`
- 所有 get/set 操作使用 `lock_guard` 保护

### 1.3 ToolsMain.cs if-else 链改为字典分发

**Files:**
- Modify: `JonysandMHDanmuTools/ToolsMain.cs`

**Status:** ✅ 已完成

**修复内容:**
- 将 25+ if-else 分支重构为字典映射 `_configHandlers`
- 符合 SOLID 开闭原则，便于扩展

### 1.4 TextToSpeech 队列操作加锁

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h`
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

**Status:** ✅ 已完成

**修复内容:**
- WebSocket 回调（HandleSpeekDm/SendGift/SC/Guard/Enter）中使用 `queueMutex_` 保护 `pendingRequests_` 队列
- 将 `AsyncTTSRequest` 存入 `shared_ptr` 管理生命周期
- 统一使用 `mutex` 替代 `recursive_mutex`，避免死锁风险

### 1.5 ConfigWindow 重复 ConfigChanged 调用

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs`

**Status:** ✅ 已完成

**修复内容:**
- `OnConnectButtonClick` 中移除重复的 `ConfigChanged` 事件通知
- 保留 `SendCommand("ConfirmIDCode:...")` 调用

---

## 2. P1 中等问题修复

### 2.1 DanmuProcessor 补充 GuardLevel 和 PaidGift 过滤

**Files:**
- Modify: `MonsterOrderWilds/DanmuProcessor.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 添加 `onlySpeekGuardLevel_ > 0` 时的 `guard_level` 过滤
- 添加 `paidGift_ > 0` 时的 `paid_gift` 过滤

### 2.2 ConfigManager SetSpeechPitch 加锁

**Files:**
- Modify: `MonsterOrderWilds/ConfigManager.cpp`

**Status:** ✅ 已完成

**修复内容:**
- `SetSpeechPitch` 方法添加 `lock_guard<mutex>` 保护

### 2.3 TTSManager 锁类型统一

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h`
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 混用 `recursive_mutex` 和 `mutex` 改为统一使用 `mutex`
- 添加 `InternalXXX` 函数供内部调用，避免锁重入问题

### 2.4 PriorityQueueManager 原子写入

**Files:**
- Modify: `MonsterOrderWilds/PriorityQueueManager.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 使用临时文件 + `std::filesystem::rename` 实现原子写入
- 避免写入中途崩溃导致文件损坏

### 2.5 Network HTTP 线程异常处理

**Files:**
- Modify: `MonsterOrderWilds/Network.cpp`

**Status:** ✅ 已完成

**修复内容:**
- HTTP 线程主体使用 try-catch 包裹
- 防止未捕获异常导致进程崩溃

---

## 3. P2 低优先级修复

### 3.1 ProfileManager 跨年计算修复

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 移除死代码 `year == lastYear + 2` 分支
- 正确处理跨年连续打卡计算

### 3.2 BliveManager 魔法数字常量提取

**Files:**
- Modify: `MonsterOrderWilds/BliveManager.h`
- Modify: `MonsterOrderWilds/BliveManager.cpp`

**Status:** ✅ 已完成

**修复内容:**
- 提取 `1000` → `RECONNECT_DELAY_MS`
- 提取 `2000` → `WS_HEARTBEAT_RECONNECT_DELAY_MS`
- 提取 `5000` → `INITIAL_HEARTBEAT_DELAY_MS`

---

## JSON 格式任务映射

```json
{
  "tasks_from_scenarios": [],
  "files_to_modify": [
    "MonsterOrderWilds/StringProcessor.h",
    "MonsterOrderWilds/StringProcessor.cpp",
    "MonsterOrderWilds/CredentialsManager.cpp",
    "JonysandMHDanmuTools/ToolsMain.cs",
    "MonsterOrderWilds/TextToSpeech.h",
    "MonsterOrderWilds/TextToSpeech.cpp",
    "JonysandMHDanmuTools/ConfigWindow.xaml.cs",
    "MonsterOrderWilds/DanmuProcessor.cpp",
    "MonsterOrderWilds/ConfigManager.cpp",
    "MonsterOrderWilds/PriorityQueueManager.cpp",
    "MonsterOrderWilds/Network.cpp",
    "MonsterOrderWilds/ProfileManager.cpp",
    "MonsterOrderWilds/BliveManager.h",
    "MonsterOrderWilds/BliveManager.cpp"
  ]
}
```

---

## 覆盖情况验证

| 问题类型 | 数量 | 状态 |
|----------|------|------|
| P0 严重问题 | 5 | ✅ 全部修复 |
| P1 中等问题 | 5 | ✅ 全部修复 |
| P2 低优先级 | 2 | ✅ 全部修复 |
| **总计** | **12** | **✅ 全部完成** |

**编译验证:** ✅ 0 个错误