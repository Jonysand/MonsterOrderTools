# 实现任务

## Feature 1: TTS缓存管理器

### Task 1.1: 创建TTSCacheManager类

**文件**: `MonsterOrderWilds/TTSCacheManager.h`

```cpp
#pragma once
#include "framework.h"
#include <string>
#include <vector>

class TTSCacheManager {
public:
    static TTSCacheManager& Inst();
    
    void Initialize();
    std::wstring GetTodayCacheDir() const;
    std::wstring GetContentPrefix(const std::string& text) const;
    bool SaveCachedAudio(const std::string& text, const std::vector<uint8_t>& audioData);
    void CleanupOldCache(int daysToKeep = 7);

private:
    TTSCacheManager() = default;
    ~TTSCacheManager() = default;
    
    std::wstring GetCacheBaseDir() const;
    std::string GetTodayDateStr() const;
};
```

**文件**: `MonsterOrderWilds/TTSCacheManager.cpp`

```cpp
#include "TTSCacheManager.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "ConfigManager.h"
#include <Shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma comment(lib, "Shlwapi.lib")

IMPLEMENT_SINGLETON(TTSCacheManager)

std::wstring TTSCacheManager::GetCacheBaseDir() const {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.rfind(L'\\');
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    return exeDir + L"\\TempAudio";
}

std::string TTSCacheManager::GetTodayDateStr() const {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
    return std::string(buffer);
}

std::wstring TTSCacheManager::GetTodayCacheDir() const {
    std::string today = GetTodayDateStr();
    return GetCacheBaseDir() + L"\\" + utf8_to_wstring(today);
}

std::wstring TTSCacheManager::GetContentPrefix(const std::string& text) const {
    std::wstring wtext = utf8_to_wstring(text);
    
    std::wstring searchTarget = L" 说：";
    size_t pos = wtext.find(searchTarget);
    if (pos != std::wstring::npos) {
        std::wstring username = wtext.substr(0, pos);
        size_t contentStart = pos + searchTarget.length();
        std::wstring afterSpeaker = wtext.substr(contentStart);
        std::wstring first5 = afterSpeaker.substr(0, std::min<size_t>(5, afterSpeaker.length()));
        return username + L"_" + first5;
    }
    
    std::wstring first5 = wtext.substr(0, std::min<size_t>(5, wtext.length()));
    return first5;
}

bool TTSCacheManager::SaveCachedAudio(const std::string& text, const std::vector<uint8_t>& audioData) {
    std::wstring todayDir = GetTodayCacheDir();
    if (!std::filesystem::exists(todayDir)) {
        if (!CreateDirectoryW(todayDir.c_str(), NULL)) {
            DWORD dirError = ::GetLastError();
            if (dirError != ERROR_ALREADY_EXISTS) {
                LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache directory"));
                return false;
            }
        }
    }
    
    std::wstring prefix = GetContentPrefix(text);
    
    int64_t timestamp = GetTickCount64();
    std::wstring fileName = prefix + L"_" + std::to_wstring(timestamp) + L".mp3";
    std::wstring filePath = todayDir + L"\\" + fileName;
    
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(TEXT("TTSCacheManager: Failed to create cache file"));
        return false;
    }
    
    DWORD bytesWritten;
    if (!WriteFile(hFile, audioData.data(), static_cast<DWORD>(audioData.size()), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        LOG_ERROR(TEXT("TTSCacheManager: Failed to write cache data"));
        return false;
    }
    
    CloseHandle(hFile);
    LOG_INFO(TEXT("TTSCacheManager: Cached audio to %s"), filePath.c_str());
    return true;
}

void TTSCacheManager::CleanupOldCache(int daysToKeep) {
    std::wstring baseDir = GetCacheBaseDir();
    if (!std::filesystem::exists(baseDir)) {
        return;
    }
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    int64_t cutoffTime = 0;
    
    FILETIME ftNow;
    SystemTimeToFileTime(&st, &ftNow);
    ULARGE_INTEGER ulNow = {ftNow.dwLowDateTime, ftNow.dwHighDateTime};
    ulNow.QuadPart -= (static_cast<ULONGLONG>(daysToKeep) * 24 * 60 * 60 * 10000000);
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((baseDir + L"\\*.*").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring dirName = findData.cFileName;
            if (dirName == L"." || dirName == L"..") continue;
            
            FILETIME ftCreate = findData.ftCreationTime;
            ULARGE_INTEGER ulCreate = {ftCreate.dwLowDateTime, ftCreate.dwHighDateTime};
            
            if (ulCreate.QuadPart < ulNow.QuadPart) {
                std::wstring dirPath = baseDir + L"\\" + dirName;
                LOG_INFO(TEXT("TTSCacheManager: Deleting old cache directory %s"), dirPath.c_str());
                
                WIN32_FIND_DATAW subFindData;
                HANDLE hSubFind = FindFirstFileW((dirPath + L"\\*.*").c_str(), &subFindData);
                if (hSubFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (!(subFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            std::wstring filePath = dirPath + L"\\" + subFindData.cFileName;
                            DeleteFileW(filePath.c_str());
                        }
                    } while (FindNextFileW(hSubFind, &subFindData));
                    FindClose(hSubFind);
                }
                
                RemoveDirectoryW(dirPath.c_str());
            }
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
}

void TTSCacheManager::Initialize() {
    const auto& config = ConfigManager::Inst()->GetConfig();
    LOG_INFO(TEXT("TTSCacheManager: Initializing with %d days to keep"), config.ttsCacheDaysToKeep);
    CleanupOldCache(config.ttsCacheDaysToKeep);
}
```

**验证**: 编译通过

---

### Task 1.2: 添加TTS缓存配置字段

**文件**: `MonsterOrderWilds/ConfigManager.h` - 在 `ConfigData` 中添加

```cpp
    // TTS缓存配置
    int ttsCacheDaysToKeep = 7;
```

添加 setter 方法声明：
```cpp
    void SetTtsCacheDaysToKeep(int value);
```

**文件**: `MonsterOrderWilds/ConfigManager.cpp` - LoadConfig 添加

```cpp
        if (j.contains("TTS_CACHE_DAYS_TO_KEEP")) config_.ttsCacheDaysToKeep = j["TTS_CACHE_DAYS_TO_KEEP"].get<int>();
```

**文件**: `MonsterOrderWilds/ConfigManager.cpp` - SaveConfig 添加

```cpp
        j["TTS_CACHE_DAYS_TO_KEEP"] = config_.ttsCacheDaysToKeep;
```

**文件**: `MonsterOrderWilds/ConfigManager.cpp` - setter 实现

```cpp
void ConfigManager::SetTtsCacheDaysToKeep(int value)
{
    std::lock_guard<Lock> lock(lock_);
    bool changed = config_.ttsCacheDaysToKeep != value;
    if (changed) { config_.ttsCacheDaysToKeep = value; dirty_ = true; }
}
```

**文件**: `MonsterOrderWilds/ConfigFieldRegistry.cpp` - 注册

```cpp
    REGISTER_FIELD("ttsCacheDaysToKeep", int, ttsCacheDaysToKeep, ConfigFieldType::Int);
```

**文件**: `MonsterOrderWilds/DataBridgeWrapper.h` - ConfigProxy 添加属性

```cpp
property int TtsCacheDaysToKeep;
```

Refresh() 和 Apply() 对应添加。

**验证**: 配置能正常保存/加载

---

### Task 1.3: 集成TTS缓存到TextToSpeech

**文件**: `MonsterOrderWilds/TextToSpeech.h` - 添加头文件

```cpp
#include "TTSCacheManager.h"
```

**文件**: `MonsterOrderWilds/TextToSpeech.cpp` - TTSManager 构造函数初始化

在 `#if USE_MIMO_TTS` 块中添加：
```cpp
TTSCacheManager::Inst().Initialize();
```

**文件**: `MonsterOrderWilds/TextToSpeech.cpp` - API返回后保存缓存

在 `req.state = AsyncTTSState::Playing;` 之后添加：

```cpp
            if (response.success && !response.audioData.empty()) {
                req.audioData = response.audioData;
                req.state = AsyncTTSState::Playing;
                LOG_INFO(TEXT("TTS Async: API request succeeded, starting playback"));
                
                std::string utf8Text = wstring_to_utf8(req.text);
                TTSCacheManager::Inst().SaveCachedAudio(utf8Text, response.audioData);
            }
```

**验证**: 相同文本第二次请求直接播放缓存

---

## C# 层实现（TtsCacheDaysToKeep配置）

### Task 1.4: 添加TtsCacheDaysToKeep到DataStructures.cs

**文件**: `JonysandMHDanmuTools/DataStructures.cs`

在 `ConfigDataSnapshot` 结构体中添加字段：
```csharp
        // TTS缓存配置
        public int TtsCacheDaysToKeep;
```

在 `FromMainConfig` 方法中添加：
```csharp
                TtsCacheDaysToKeep = config.TTS_CACHE_DAYS_TO_KEEP,
```

在 `ApplyTo` 方法中添加：
```csharp
            config.TTS_CACHE_DAYS_TO_KEEP = TtsCacheDaysToKeep;
```

---

### Task 1.5: 注册TtsCacheDaysToKeep字段到Utils.cs

**文件**: `JonysandMHDanmuTools/Utils.cs`

在 `ConfigFieldRegistry` 静态构造函数中添加：
```csharp
            Register("ttsCacheDaysToKeep", ConfigFieldType.Int,
                () => GetInt("ttsCacheDaysToKeep"),
                v => SetValue("ttsCacheDaysToKeep", (int)v, ConfigFieldType.Int));
```

---

### Task 1.6: 添加TtsCacheDaysToKeep到ProxyClasses.cs

**文件**: `JonysandMHDanmuTools/ProxyClasses.cs`

在 `ConfigProxy` 类中添加属性：
```csharp
        private int _ttsCacheDaysToKeep = 7;
        public int TtsCacheDaysToKeep
        {
            get => _ttsCacheDaysToKeep;
            set { _ttsCacheDaysToKeep = value; OnPropertyChanged(); }
        }
```

在 `RefreshFromConfig` 方法中添加：
```csharp
            TtsCacheDaysToKeep = config.TTS_CACHE_DAYS_TO_KEEP;
```

在 `ApplyToConfig` 方法中添加：
```csharp
            config.TTS_CACHE_DAYS_TO_KEEP = TtsCacheDaysToKeep;
```

---

### Task 1.7: 添加UI控件到ConfigWindow.xaml

**文件**: `JonysandMHDanmuTools/ConfigWindow.xaml`

在 TTS设置区域（MimoSpeedSlider之后）添加：
```xml
                                    <StackPanel Orientation="Horizontal" Margin="0,0,0,4">
                                        <Label
                                            Content="缓存保留天数"
                                            VerticalAlignment="Center"
                                            FontSize="14"
                                            Foreground="#222"
                                            Margin="0,0,8,0" />
                                        <TextBox
                                            Name="TtsCacheDaysToKeepTextBox"
                                            Width="60"
                                            FontSize="14"
                                            VerticalAlignment="Center"
                                            TextChanged="TtsCacheDaysToKeepTextBox_TextChanged" />
                                        <Label
                                            Content="天"
                                            VerticalAlignment="Center"
                                            FontSize="14"
                                            Foreground="#222"
                                            Margin="4,0,0,0" />
                                    </StackPanel>
```

---

### Task 1.8: 添加事件处理到ConfigWindow.xaml.cs

**文件**: `JonysandMHDanmuTools/ConfigWindow.xaml.cs`

添加事件处理方法：
```csharp
private void TtsCacheDaysToKeepTextBox_TextChanged(object sender, TextChangedEventArgs e)
{
    if (TtsCacheDaysToKeepTextBox == null) return;
    
    if (int.TryParse(TtsCacheDaysToKeepTextBox.Text, out int days))
    {
        if (days < 1) days = 1;
        if (days > 365) days = 365;
        
        if (ConfigProxy.Instance.TtsCacheDaysToKeep != days)
        {
            ConfigProxy.Instance.TtsCacheDaysToKeep = days;
            IsConfigModified = true;
        }
    }
}
```

在 `FillConfig` 方法中添加初始化：
```csharp
            TtsCacheDaysToKeepTextBox.Text = ConfigProxy.Instance.TtsCacheDaysToKeep.ToString();
```

---

## Feature 2: 礼物连击优化

### Task 2.1: 添加DynamicComboEntry结构

**文件**: `MonsterOrderWilds/TextToSpeech.h` - 添加

```cpp
#if USE_MIMO_TTS
    struct DynamicComboEntry {
        std::string combo_id;
        std::string uname;
        std::string gift_name;
        int gift_num;
        float combo_timeout;
        bool paid;
        bool firstReported;
        int64_t lastUpdateTime;
        
        DynamicComboEntry() : gift_num(0), combo_timeout(0), paid(false), firstReported(false), lastUpdateTime(0) {}
    };
    
    std::unordered_map<std::string, DynamicComboEntry> dynamicComboMap_;
    std::unordered_map<std::string, int64_t> giftCooldownMap_;
    
    bool IsInCooldown(const std::string& comboId);
    void UpdateCooldown(const std::string& comboId);
    void CleanupExpiredCooldowns();
#endif
```

---

### Task 2.2: 实现冷却时间管理

**文件**: `MonsterOrderWilds/TextToSpeech.cpp` - 添加常量和方法

```cpp
#if USE_MIMO_TTS
constexpr int GIFT_COOLDOWN_SECONDS = 5;
constexpr int DYNAMIC_COMBO_WINDOW_SECONDS = 10;

bool TTSManager::IsInCooldown(const std::string& comboId) {
    auto it = giftCooldownMap_.find(comboId);
    if (it == giftCooldownMap_.end()) return false;
    
    int64_t cooldownMs = GIFT_COOLDOWN_SECONDS * 1000;
    return (GetTickCount64() - it->second) < cooldownMs;
}

void TTSManager::UpdateCooldown(const std::string& comboId) {
    giftCooldownMap_[comboId] = GetTickCount64();
}

void TTSManager::CleanupExpiredCooldowns() {
    int64_t cooldownMs = GIFT_COOLDOWN_SECONDS * 1000;
    int64_t now = GetTickCount64();
    for (auto it = giftCooldownMap_.begin(); it != giftCooldownMap_.end(); ) {
        if (now - it->second > cooldownMs * 2) {
            it = giftCooldownMap_.erase(it);
        } else ++it;
    }
}
#endif
```

---

### Task 2.3: 修改HandleSpeekSendGift

**文件**: `MonsterOrderWilds/TextToSpeech.cpp` - 完全重写 HandleSpeekSendGift

```cpp
void TTSManager::HandleSpeekSendGift(const json& data)
{
    const auto& paid = data["paid"].get<bool>();
    const auto& uname = data["uname"].get<std::string>();
    const auto& gift_name = data["gift_name"].get<std::string>();
    int gift_num = data["gift_num"].get<int>();
    const auto& open_id = data["open_id"].get<std::string>();
    std::string gift_id = std::to_string(data["gift_id"].get<int>());
    std::string combo_id = open_id + gift_id;

    if (IsInCooldown(combo_id)) {
        auto it = dynamicComboMap_.find(combo_id);
        if (it != dynamicComboMap_.end()) {
            it->second.gift_num += gift_num;
            it->second.lastUpdateTime = GetTickCount64();
            it->second.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
        }
        return;
    }

    if (paid && data.contains("combo_info")) {
        std::string official_combo_id = data["combo_info"]["combo_id"].get<std::string>();
        int combo_timeout = data["combo_info"]["combo_timeout"].get<int>();
        int combo_base_num = data["combo_info"]["combo_base_num"].get<int>();
        int combo_count = data["combo_info"]["combo_count"].get<int>();
        gift_num = combo_base_num * combo_count;
        
        auto it = ComboGiftMsgPrepareMap.find(official_combo_id);
        if (it != ComboGiftMsgPrepareMap.end()) {
            it->second.combo_timeout = combo_timeout;
            it->second.gift_num = gift_num;
            if (!it->second.firstReported) {
                TString firstMsg = TEXT("感谢 ") + utf8_to_wstring(uname) + 
                                   TEXT(" 开始赠送") + utf8_to_wstring(gift_name);
                GiftMsgQueue.push_back(firstMsg);
                HistoryLogMsgQueue.push_back(firstMsg);
                it->second.firstReported = true;
                UpdateCooldown(combo_id);
            }
        } else {
            GiftComboInfo info;
            info.uname = uname;
            info.gift_name = gift_name;
            info.gift_num = gift_num;
            info.combo_timeout = combo_timeout;
            info.paid = paid;
            info.firstReported = false;
            ComboGiftMsgPrepareMap.emplace(official_combo_id, std::move(info));
            
            TString firstMsg = TEXT("感谢 ") + utf8_to_wstring(uname) + 
                               TEXT(" 开始赠送") + utf8_to_wstring(gift_name);
            GiftMsgQueue.push_back(firstMsg);
            HistoryLogMsgQueue.push_back(firstMsg);
            UpdateCooldown(combo_id);
            
            auto findIt = ComboGiftMsgPrepareMap.find(official_combo_id);
            if (findIt != ComboGiftMsgPrepareMap.end()) {
                findIt->second.firstReported = true;
            }
        }
    } else {
        auto it = dynamicComboMap_.find(combo_id);
        if (it != dynamicComboMap_.end()) {
            it->second.gift_num += gift_num;
            it->second.lastUpdateTime = GetTickCount64();
            it->second.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
            
            if (!it->second.firstReported && it->second.gift_num >= 3) {
                TString firstMsg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) + 
                                   TEXT(" 开始赠送") + utf8_to_wstring(it->second.gift_name);
                GiftMsgQueue.push_back(firstMsg);
                HistoryLogMsgQueue.push_back(firstMsg);
                it->second.firstReported = true;
                UpdateCooldown(combo_id);
            }
        } else {
            DynamicComboEntry entry;
            entry.combo_id = combo_id;
            entry.uname = uname;
            entry.gift_name = gift_name;
            entry.gift_num = gift_num;
            entry.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
            entry.paid = paid;
            entry.firstReported = false;
            entry.lastUpdateTime = GetTickCount64();
            dynamicComboMap_.emplace(combo_id, std::move(entry));
            
            if (gift_num < 3) {
                TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 赠送的") + 
                              std::to_wstring(gift_num) + TEXT("个") + utf8_to_wstring(gift_name);
                GiftMsgQueue.push_back(msg);
                HistoryLogMsgQueue.push_back(msg);
                UpdateCooldown(combo_id);
                
                auto findIt = dynamicComboMap_.find(combo_id);
                if (findIt != dynamicComboMap_.end()) {
                    findIt->second.firstReported = true;
                }
            }
        }
    }
}
```

---

### Task 2.4: 修改Tick处理动态连击

**文件**: `MonsterOrderWilds/TextToSpeech.cpp` - 在 Tick() 循环后添加

```cpp
    for (auto it = dynamicComboMap_.begin(); it != dynamicComboMap_.end(); ) {
        it->second.combo_timeout -= deltaTime;
        if (it->second.combo_timeout <= 0.0f) {
            if (!it->second.firstReported || it->second.gift_num > 0) {
                TString msg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) + 
                              TEXT(" 赠送的") + std::to_wstring(it->second.gift_num) + 
                              TEXT("个") + utf8_to_wstring(it->second.gift_name);
                GiftMsgQueue.push_back(msg);
                HistoryLogMsgQueue.push_back(msg);
            }
            it = dynamicComboMap_.erase(it);
        } else {
            ++it;
        }
    }
    
    static int64_t lastCooldownCleanup = 0;
    if (GetTickCount64() - lastCooldownCleanup > 60000) {
        CleanupExpiredCooldowns();
        lastCooldownCleanup = GetTickCount64();
    }
```

---

## 测试

### Task 3.1: TTSCacheManager单元测试

**文件**: `MonsterOrderWilds/TTSCacheManagerTests.cpp`

```cpp
#ifdef RUN_UNIT_TESTS
#include "TTSCacheManager.h"
#include "ConfigManager.h"
#include <cassert>
#include <iostream>
#include <filesystem>

void TestTTSCacheManager() {
    std::cout << "[TestTTSCacheManager] Starting tests..." << std::endl;
    
    ConfigManager::CreateInstance();
    ConfigManager::Inst()->LoadConfig();
    
    TTSCacheManager::CreateInstance();
    TTSCacheManager::Inst().Initialize();
    
    std::wstring baseDir = TTSCacheManager::Inst().GetCacheBaseDir();
    assert(!baseDir.empty());
    std::cout << "[PASS] GetCacheBaseDir" << std::endl;
    
    std::wstring todayDir = TTSCacheManager::Inst().GetTodayCacheDir();
    assert(!todayDir.empty());
    std::cout << "[PASS] GetTodayCacheDir" << std::endl;
    
    std::string testText = "小明 说：你好啊朋友";
    std::string prefix = TTSCacheManager::Inst().GetContentPrefix(testText);
    assert(prefix == "小明_你好啊朋");
    std::cout << "[PASS] GetContentPrefix with speaker" << std::endl;
    
    testText = "感谢用户赠送的礼物";
    prefix = TTSCacheManager::Inst().GetContentPrefix(testText);
    assert(prefix == "感谢用户赠");
    std::cout << "[PASS] GetContentPrefix without speaker" << std::endl;
    
    std::vector<uint8_t> testAudio = {0x00, 0x01, 0x02, 0x03};
    bool saveResult = TTSCacheManager::Inst().SaveCachedAudio(testText, testAudio);
    assert(saveResult == true);
    assert(std::filesystem::exists(todayDir));
    std::cout << "[PASS] SaveCachedAudio" << std::endl;
    
    TTSCacheManager::Inst().CleanupOldCache(7);
    std::cout << "[PASS] CleanupOldCache" << std::endl;
    
    std::cout << "[PASS] All TTSCacheManager tests passed!" << std::endl;
}
#endif
```

---

### Task 3.2: 礼物连击单元测试

**文件**: `MonsterOrderWilds/TextToSpeechTests.cpp`

```cpp
#ifdef RUN_UNIT_TESTS
#include "TextToSpeech.h"
#include <cassert>
#include <iostream>

void TestDynamicComboConstants() {
    std::cout << "[TestDynamicComboConstants] Starting tests..." << std::endl;
    
    assert(TTSManager::GIFT_COOLDOWN_SECONDS == 5);
    std::cout << "[PASS] GIFT_COOLDOWN_SECONDS == 5" << std::endl;
    
    assert(TTSManager::DYNAMIC_COMBO_WINDOW_SECONDS == 10);
    std::cout << "[PASS] DYNAMIC_COMBO_WINDOW_SECONDS == 10" << std::endl;
    
    TTSManager::DynamicComboEntry entry;
    assert(entry.gift_num == 0);
    assert(entry.combo_timeout == 0.0f);
    std::cout << "[PASS] DynamicComboEntry default constructor" << std::endl;
    
    entry.gift_num = 10;
    entry.uname = "test_user";
    entry.gift_name = "辣条";
    entry.combo_timeout = 10.0f;
    entry.paid = true;
    entry.firstReported = true;
    entry.lastUpdateTime = 1234567890;
    assert(entry.gift_num == 10);
    assert(entry.uname == "test_user");
    std::cout << "[PASS] DynamicComboEntry field assignment" << std::endl;
    
    std::cout << "[PASS] All dynamic combo constants tests passed!" << std::endl;
}
#endif
```

**注意**：冷却逻辑（IsInCooldown/UpdateCooldown/CleanupExpiredCooldowns）是 private 方法，需要通过集成测试验证。

---

### Task 3.2: 礼物连击单元测试

**文件**: `MonsterOrderWilds/TextToSpeechTests.cpp`

```cpp
#ifdef RUN_UNIT_TESTS
#include "TextToSpeech.h"
#include <cassert>
#include <iostream>

void TestDynamicComboConstants() {
    std::cout << "[TestDynamicComboConstants] Starting tests..." << std::endl;
    
    assert(GIFT_COOLDOWN_SECONDS == 5);
    std::cout << "[PASS] GIFT_COOLDOWN_SECONDS == 5" << std::endl;
    
    assert(DYNAMIC_COMBO_WINDOW_SECONDS == 10);
    std::cout << "[PASS] DYNAMIC_COMBO_WINDOW_SECONDS == 10" << std::endl;
    
    std::cout << "[PASS] All dynamic combo constants tests passed!" << std::endl;
}
#endif
```

---

## 编译验证

### Task 4.1: MSBuild编译

**命令**:
```bash
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln" -p:Configuration=Release -p:Platform=x64 -t:Build -m
```

**预期**: 编译成功，无错误

---

## 文件清单

| 操作 | 文件 |
|------|------|
| 新增 | `MonsterOrderWilds/TTSCacheManager.cpp` |
| 新增 | `MonsterOrderWilds/TTSCacheManager.h` |
| 新增 | `MonsterOrderWilds/TTSCacheManagerTests.cpp` |
| 修改 | `MonsterOrderWilds/ConfigManager.h` |
| 修改 | `MonsterOrderWilds/ConfigManager.cpp` |
| 修改 | `MonsterOrderWilds/ConfigFieldRegistry.cpp` |
| 修改 | `MonsterOrderWilds/DataBridgeWrapper.h` |
| 修改 | `MonsterOrderWilds/TextToSpeech.h` |
| 修改 | `MonsterOrderWilds/TextToSpeech.cpp` |
| 修改 | `JonysandMHDanmuTools/DataStructures.cs` |
| 修改 | `JonysandMHDanmuTools/Utils.cs` |
| 修改 | `JonysandMHDanmuTools/ProxyClasses.cs` |
| 修改 | `JonysandMHDanmuTools/ConfigWindow.xaml` |
| 修改 | `JonysandMHDanmuTools/ConfigWindow.xaml.cs` |

---

## 实现顺序

1. **C++ 层实现**：
   - Task 1.1: TTSCacheManager 类
   - Task 1.2: ConfigManager 配置字段
   - Task 1.3: TextToSpeech 集成
   - Task 2.1: DynamicComboEntry 结构
   - Task 2.2: 冷却时间管理
   - Task 2.3: HandleSpeekSendGift 修改
   - Task 2.4: Tick 处理修改

2. **C# 层实现**：
   - Task 1.4: DataStructures.cs
   - Task 1.5: Utils.cs
   - Task 1.6: ProxyClasses.cs
   - Task 1.7: ConfigWindow.xaml
   - Task 1.8: ConfigWindow.xaml.cs

3. **测试与验证**：
   - Task 3.1: TTSCacheManager 单元测试
   - Task 3.2: 礼物连击单元测试
   - Task 4.1: MSBuild 编译验证
