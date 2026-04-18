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

### Bug 4: 所有配置字段初始化时覆盖保存值 (P1 High)

**Status:** ✅ 已完成

**Root Cause:** FillConfig 设置控件值时触发事件，覆盖保存的配置。`_isInitializing` 只在 TTSEngineComboBox 中添加了，其他 12 个事件处理器都没有保护。

**Fix:**
1. ✅ `ConfigWindow.xaml` - 移除所有 XAML 硬编码初始值（Slider `Value=`，ComboBoxItem `IsSelected="True"`）
2. ✅ `ConfigWindow.xaml.cs` - 为所有 13 个事件处理器添加 `if (_isInitializing) return;`
   - VoiceRateSlider_ValueChanged
   - VoicePitchSlider_ValueChanged
   - VoiceVolumeSlider_ValueChanged
   - OnlyMedalCheckBox_Changed
   - OnlyGuardLevel_SelectionChanged
   - OnlyPaidGiftCheckBox_Changed
   - OpacitySlider_ValueChanged
   - PenetratingModeOpacitySlider_ValueChanged
   - MimoVoiceComboBox_SelectionChanged
   - MimoStyleComboBox_SelectionChanged
   - MiniMaxVoiceComboBox_SelectionChanged
   - MiniMaxSpeedSlider_ValueChanged

---

### Bug 5: MINIMAX_SPEED/MINIMAX_VOICE_ID 未持久化 (P1 High)

**Status:** ✅ 已完成

**Root Cause:** ConfigData 中存在这两个字段，ConfigFieldRegistry 中注册了，但 ConfigManager 的 LoadConfig/SaveConfig 中漏了持久化处理。

**Fix:**
1. ✅ `ConfigManager.cpp` - LoadConfig 添加 `MINIMAX_VOICE_ID` 和 `MINIMAX_SPEED` 读取
2. ✅ `ConfigManager.cpp` - SaveConfig 添加 `MINIMAX_VOICE_ID` 和 `MINIMAX_SPEED` 写入

---

### 清理: 删除无用的 mimoSpeed 字段 (P2 Low)

**Status:** ✅ 已完成

**确认:** `mimoSpeed` 从未在任何 TTS provider 中实际使用，仅存在于配置层。

**删除内容:**
1. ✅ `ConfigManager.h` - 删除 `float mimoSpeed` 字段和 `SetMimoSpeed()` 声明
2. ✅ `ConfigManager.cpp` - 删除 LoadConfig/SaveConfig 中的 `MIMO_SPEED` 处理和 `SetMimoSpeed()` 函数
3. ✅ `ConfigFieldRegistry.cpp` - 删除 `REGISTER_FIELD("mimoSpeed", ...)`
4. ✅ `DataBridgeWrapper.h` - 删除两处 `MimoSpeed` 绑定
5. ✅ `ConfigManagerTests.cpp` - 删除 `mimoSpeed` 相关测试断言

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
    "MonsterOrderWilds/ConfigManager.cpp",
    "MonsterOrderWilds/ConfigFieldRegistry.cpp",
    "MonsterOrderWilds/DataBridgeWrapper.h",
    "MonsterOrderWilds/ConfigManagerTests.cpp",
    "installer/MonsterOrderWilds.iss"
  ]
}
```

---

## 编译验证

✅ MSBuild Release x64 编译通过，0 个错误
✅ 版本更新 v20 → v21