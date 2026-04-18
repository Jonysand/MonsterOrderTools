# TTS 引擎选择修复 - Implementation Tasks

## Bug 列表

### Bug 1: std::lock_guard SEHException (P0 Critical)

**Status:** ✅ 已完成

**Root Cause:** `asyncMutex_` 使用 `std::mutex` 不支持递归锁定

**Fix:**
1. ✅ `TextToSpeech.h:149` - `std::mutex asyncMutex_` → `std::recursive_mutex asyncMutex_`
2. ✅ `TextToSpeech.cpp` - 8 处 `std::lock_guard<std::mutex>` → `std::lock_guard<std::recursive_mutex>`
   - Line 330: Speak()
   - Line 360: SpeakCheckinTTS()
   - Line 504: SapiSpeakCallback()
   - Line 566: SpeakWithMimoAsync()
   - Line 576: ProcessAsyncTTS()
   - Line 744: ProcessPendingRequestInternal() lambda
   - Line 913: CleanupCompletedRequests()

---

### Bug 2: XiaomiTTSProvider::HashtagToStyle 迭代器失效 (P0 Critical)

**Status:** ✅ 已完成

**Root Cause:** `result.replace()` 后迭代器失效，计算新的 searchStart 时混合了修改前后的迭代器

**Fix:**
1. ✅ `XiaomiTTSProvider.cpp:190-202` - 重写 HashtagToStyle 函数
   - 使用 `size_t searchPos` 替代 `std::string::const_iterator`
   - 使用 `match.position()` 获取相对偏移
   - 使用 `result.begin() + matchStart` 计算绝对位置

---

### Bug 3: TTS 引擎选择无法持久化 (P1 High)

**Status:** ✅ 已完成

**Root Cause:**
1. XAML `IsSelected="True"` 在窗口加载时触发 SelectionChanged 事件
2. switch 语句索引与 XAML ComboBoxItem 顺序不匹配

**Fix:**
1. ✅ `ConfigWindow.xaml:212-222` - 移除 `IsSelected="True"`，改用 `SelectedIndex="0"`
2. ✅ `ConfigWindow.xaml.cs:14-18` - 添加 `_isInitializing` 标志和 `Loaded` 事件处理
3. ✅ `ConfigWindow.xaml.cs:68-79` - 修正 switch 语句索引
   - `"minimax"` → `SelectedIndex = 1`
   - `"mimo"` → `SelectedIndex = 2`
   - `"sapi"` → `SelectedIndex = 3`
4. ✅ `ConfigWindow.xaml.cs:338` - 添加 `if (_isInitializing) return;`

---

## 版本更新

**Status:** ✅ 已完成

- ✅ `framework.h:50` - `APP_VERSION 20` → `APP_VERSION 21`
- ✅ `VersionInfo.rc:6-7` - `FILEVERSION 20,0,0,0` → `FILEVERSION 21,0,0,0`
- ✅ `VersionInfo.rc:24-25` - `FileVersion/ProductVersion "20"` → `"21"`
- ✅ `installer/MonsterOrderWilds.iss:5` - `MyAppVersion "v20"` → `MyAppVersion "v21"`

---

## 修改文件清单

```json
{
  "files_modified": [
    "MonsterOrderWilds/TextToSpeech.h",
    "MonsterOrderWilds/TextToSpeech.cpp",
    "MonsterOrderWilds/XiaomiTTSProvider.cpp",
    "MonsterOrderWilds/framework.h",
    "MonsterOrderWilds/VersionInfo.rc",
    "JonysandMHDanmuTools/ConfigWindow.xaml",
    "JonysandMHDanmuTools/ConfigWindow.xaml.cs",
    "installer/MonsterOrderWilds.iss"
  ]
}
```

---

## 编译验证

✅ MSBuild Release x64 编译通过，0 个错误
✅ 版本更新 v20 → v21