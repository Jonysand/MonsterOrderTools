# TTS 引擎选择修复 - Design Document

## 概述

本次修复解决了 TTS 相关的三个 bug：
1. `std::lock_guard` 锁定失败导致的 SEHException 崩溃
2. `XiaomiTTSProvider::HashtagToStyle` 中迭代器失效导致的 debug break
3. TTS 引擎选择 UI 配置无法持久化的问题

## Bug 修复详情

### Bug 1: std::lock_guard SEHException

**问题描述:** `std::lock_guard<std::mutex>` 在 `ProcessAsyncTTS()` Line 576 锁定时抛出 SEHException。

**根因分析:**
- 原代码使用 `std::mutex`，不支持递归锁定
- `ProcessAsyncTTS()` 持有锁期间调用 `audioPlayer->Play()` 触发 SAPI 回调 `SapiSpeakCallback()`
- `SapiSpeakCallback()` 在另一线程执行，也需要获取同一个锁
- 即使在同一线程，`ProcessPendingRequestInternal()` 等函数可能被递归调用

**修复方案:**
- 将 `asyncMutex_` 从 `std::mutex` 改回 `std::recursive_mutex`
- 更新所有 `std::lock_guard<std::mutex>` 为 `std::lock_guard<std::recursive_mutex>`

**涉及文件:**
- `TextToSpeech.h:149` - `std::mutex asyncMutex_` → `std::recursive_mutex asyncMutex_`
- `TextToSpeech.cpp` - 8 处 `lock_guard<std::mutex>` → `lock_guard<std::recursive_mutex>`

---

### Bug 2: XiaomiTTSProvider::HashtagToStyle 迭代器失效

**问题描述:** Debug build 在 `HashtagToStyle()` Line 199 执行 `result.replace()` 后触发 `__debugbreak()`。

**根因分析:**
```cpp
// 原代码问题
std::string::const_iterator searchStart(result.cbegin());
while (std::regex_search(searchStart, result.cend(), match, pattern)) {
    result.replace(match[0].first, match[0].second, replacement);
    // searchStart 基于修改前的 result.cbegin() 计算，但 result 已改变
    searchStart = result.cbegin() + (...);
}
```
- `result.replace()` 后迭代器失效
- 计算新的 `searchStart` 时混合了修改前后的迭代器

**修复方案:**
- 改用 `size_t searchPos` 整数偏移跟踪位置
- 用 `match.position()` 获取匹配在搜索范围内的相对位置

**涉及文件:**
- `XiaomiTTSProvider.cpp:190-202` - 重写 HashtagToStyle 函数

---

### Bug 3: TTS 引擎选择无法持久化

**问题描述:** 第一次启动选择 mimo，第二次启动后 UI 显示自动。

**根因分析:**
1. XAML 中 `<ComboBoxItem Content="自动" IsSelected="True" Tag="auto" />` 在窗口加载时触发 SelectionChanged 事件
2. 事件发送 `ConfigChanged("TTS_ENGINE:auto")` 覆盖用户保存的配置
3. 同时 switch 语句索引与 XAML 中 ComboBoxItem 顺序不匹配

**修复方案:**
1. 移除 `IsSelected="True"`，改用 ComboBox 的 `SelectedIndex="0"` 属性
2. 添加 `_isInitializing` 标志阻止初始化期间的事件触发
3. 修正 switch 语句的索引映射

**涉及文件:**
- `ConfigWindow.xaml:212-222` - 改用 SelectedIndex，移除 IsSelected
- `ConfigWindow.xaml.cs:14-18` - 添加 _isInitializing 标志
- `ConfigWindow.xaml.cs:68-79` - 修正 minimax 索引
- `ConfigWindow.xaml.cs:338` - 添加 _isInitializing 检查

---

## 版本更新

本次修复将版本从 v20 更新到 v21：
- `framework.h` - `APP_VERSION 20` → `APP_VERSION 21`
- `VersionInfo.rc` - `FILEVERSION 20,0,0,0` → `FILEVERSION 21,0,0,0`
- `installer/MonsterOrderWilds.iss` - `MyAppVersion "v20"` → `MyAppVersion "v21"`

## 编译验证

✅ MSBuild Release x64 编译通过，0 个错误