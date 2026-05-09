# Manbo TTS 音色选择实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 Manbo TTS 引擎新增音色选择功能，C# UI 从 API 获取音色列表，C++ 层根据选择切换 API 端点。

**Architecture:** 新增 `manboVoice` 配置字段通过现有 ConfigFieldRegistry/DataBridge 跨 C++/C# 同步。C# 启动时异步获取音色列表并渲染 ComboBox。C++ ManboTTSProvider 根据音色值路由到 `/apis/mbAIscvip`（曼波）或 `/apis/AIvoice`（其他）。

**Tech Stack:** ["C++17", "C++/CLI", "C# WPF", "nlohmann/json", "WinHTTP", "System.Net.Http"]

---

## Task 1: C++ 配置字段 — ConfigData 与持久化

**Files:**
- Modify: `MonsterOrderWilds/ConfigManager.h:13`
- Modify: `MonsterOrderWilds/ConfigManager.cpp:188-198,278-279`
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp:28`
- Test: `MonsterOrderWilds/ConfigManagerTests.cpp`（如存在）或新增断言到现有测试

**Spec Coverage:** Requirement "Configuration field syncs across layers" — Load/Save scenarios; Requirement "Backward compatibility" — old config scenario

- [ ] **Step 1: Add manboVoice to ConfigData** (2 min)

  在 `ConfigManager.h` 的 `ConfigData` 结构体中，在 `manboApiKey` 下方添加：

```cpp
    std::string manboVoice = "曼波";
```

- [ ] **Step 2: Register field in ConfigFieldRegistry** (2 min)

  在 `ConfigFieldRegistry.cpp` 的 `RegisterAll()` 中，在 `manboApiKey` 注册下方添加：

```cpp
    REGISTER_FIELD("manboVoice", std::string, manboVoice, ConfigFieldType::String);
```

- [ ] **Step 3: Handle JSON Load/Save in ConfigManager** (5 min)

  在 `ConfigManager.cpp` 的 `LoadConfig()` 中，找到读取 JSON 配置的区域（在读取 `manboApiKey` 附近），添加：

```cpp
        if (j.contains("MANBO_VOICE") && j["MANBO_VOICE"].is_string()) {
            config_.manboVoice = j["MANBO_VOICE"].get<std::string>();
        } else {
            config_.manboVoice = "曼波"; // 默认值，兼容旧配置
        }
```

  在 `SaveConfig()` 的 JSON 写入区域，添加：

```cpp
        j["MANBO_VOICE"] = config_.manboVoice;
```

- [ ] **Step 4: Add setter to ConfigManager** (2 min)

  在 `ConfigManager.h` 中，在 `SetManboApiKey` 下方添加声明：

```cpp
    void SetManboVoice(const std::string& value);
```

  在 `ConfigManager.cpp` 末尾添加实现（参照其他 setter）：

```cpp
void ConfigManager::SetManboVoice(const std::string& value) {
    LockGuard guard(lock_);
    bool changed = config_.manboVoice != value;
    if (changed) { config_.manboVoice = value; dirty_ = true; }
}
```

- [ ] **Step 5: Compile C++ layer** (2 min)

  Run: `msbuild MonsterOrderWilds/MonsterOrderWilds.vcxproj /p:Configuration=Release /p:Platform=x64`
  Expected: 0 errors

---

## Task 2: C++/CLI DataBridge 同步

**Files:**
- Modify: `MonsterOrderWilds/DataBridgeWrapper.h:21,36,49,84`

**Spec Coverage:** Requirement "Configuration field syncs across layers"

- [ ] **Step 1: Add ManboVoice to ConfigProxy Refresh** (2 min)

  在 `DataBridgeWrapper.h` 的 `Refresh()` 方法中，在 `ManboApiKey` 行下方添加：

```cpp
            ManboVoice = gcnew System::String(config.manboVoice.c_str());
```

- [ ] **Step 2: Add ManboVoice to ConfigProxy Apply** (2 min)

  在 `Apply()` 方法中，在 `ManboApiKey` 行下方添加：

```cpp
            data.manboVoice = msclr::interop::marshal_as<std::string>(ManboVoice);
```

- [ ] **Step 3: Add property declaration** (1 min)

  在 `ConfigProxy` 的属性区域，在 `ManboApiKey` 下方添加：

```cpp
        property System::String^ ManboVoice;
```

- [ ] **Step 4: Compile C++/CLI layer** (2 min)

  Run: `msbuild MonsterOrderWilds/MonsterOrderWilds.vcxproj /p:Configuration=Release /p:Platform=x64`
  Expected: 0 errors

---

## Task 3: C# 配置层 — Utils.cs, DataStructures.cs, ProxyClasses.cs

**Files:**
- Modify: `JonysandMHDanmuTools/Utils.cs:158-160,327-330`
- Modify: `JonysandMHDanmuTools/DataStructures.cs:13,54,85`
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:33,183,209`

**Spec Coverage:** Requirement "Configuration field syncs across layers"

- [ ] **Step 1: Register MANBO_VOICE in ConfigFieldRegistry** (2 min)

  在 `Utils.cs` 的 `ConfigFieldRegistry` 静态构造函数中，在 `manboApiKey` 注册下方添加：

```csharp
            Register("manboVoice", ConfigFieldType.String,
                () => GetString("manboVoice"),
                v => SetValue("manboVoice", (string)v, ConfigFieldType.String));
```

- [ ] **Step 2: Add MANBO_VOICE property to MainConfig** (2 min)

  在 `Utils.cs` 的 `MainConfig` 类中，在 `MANBO_API_KEY` 下方添加：

```csharp
        public String MANBO_VOICE
        {
            get => (string)ConfigFieldRegistry.Get("manboVoice");
            set { ConfigFieldRegistry.Set("manboVoice", value); OnPropertyChanged(); }
        }
```

- [ ] **Step 3: Update ConfigDataSnapshot** (3 min)

  在 `DataStructures.cs` 中：
  1. 在 `ConfigDataSnapshot` 结构体中，在 `ManboApiKey` 下方添加：`public string ManboVoice;`
  2. 在 `FromMainConfig()` 中，在 `ManboApiKey` 下方添加：`ManboVoice = config.MANBO_VOICE ?? "曼波",`
  3. 在 `ApplyTo()` 中，在 `MANBO_API_KEY` 下方添加：`config.MANBO_VOICE = ManboVoice;`

- [ ] **Step 4: Update ConfigProxy in ProxyClasses.cs** (3 min)

  在 `ProxyClasses.cs` 中：
  1. 在私有字段区域添加：`private string _manboVoice = "曼波";`
  2. 在属性区域添加：

```csharp
        public string ManboVoice
        {
            get => _manboVoice;
            set { _manboVoice = value; OnPropertyChanged(); }
        }
```

  3. 在 `RefreshFromConfig()` 中，在 `ManboApiKey` 下方添加：`ManboVoice = config.MANBO_VOICE ?? "曼波";`
  4. 在 `ApplyToConfig()` 中，在 `MANBO_API_KEY` 下方添加：`config.MANBO_VOICE = ManboVoice;`

- [ ] **Step 5: Build C# layer** (2 min)

  Run: `msbuild JonysandMHDanmuTools/JonysandMHDanmuTools.csproj /p:Configuration=Release /p:Platform=x64`
  Expected: 0 errors

---

## Task 4: C# 音色列表异步获取

**Files:**
- Modify: `JonysandMHDanmuTools/ToolsMain.xaml.cs`（或 `ToolsMain.cs`）

**Spec Coverage:** Requirement "C# UI can fetch and display voice list" — all 3 scenarios

- [ ] **Step 1: Add voice list cache and fetch method** (5 min)

  在 `ToolsMain.xaml.cs`（或 `ToolsMain.cs`）的 `ToolsMain` 类中，添加：

```csharp
    public static List<string> ManboVoiceList { get; private set; } = new List<string> { "曼波" };

    private async void FetchManboVoiceListAsync()
    {
        try
        {
            using (var client = new System.Net.Http.HttpClient())
            {
                client.Timeout = TimeSpan.FromSeconds(10);
                var response = await client.GetStringAsync("https://api.milorapart.top/apis/AIvoice/?type=list");
                var json = System.Text.Json.JsonDocument.Parse(response);
                if (json.RootElement.TryGetProperty("speakers", out var speakersElement) && speakersElement.ValueKind == System.Text.Json.JsonValueKind.Array)
                {
                    var voices = new List<string> { "曼波" };
                    foreach (var speaker in speakersElement.EnumerateArray())
                    {
                        voices.Add(speaker.GetString());
                    }
                    ManboVoiceList = voices;
                }
            }
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"FetchManboVoiceList failed: {ex.Message}");
            // 失败时保持默认列表 ["曼波"]
        }
    }
```

- [ ] **Step 2: Call fetch on startup** (2 min)

  在 `ToolsMain` 的构造函数或 `OnStartup` 中，在初始化完成后调用：

```csharp
    FetchManboVoiceListAsync();
```

- [ ] **Step 3: Build C# layer** (2 min)

  Run: `msbuild JonysandMHDanmuTools/JonysandMHDanmuTools.csproj /p:Configuration=Release /p:Platform=x64`
  Expected: 0 errors

---

## Task 5: C++ ManboTTSProvider 端点切换

**Files:**
- Modify: `MonsterOrderWilds/ManboTTSProvider.cpp:40-51,146-195`
- Modify: `MonsterOrderWilds/ManboTTSProviderTests.cpp`（新增测试）

**Spec Coverage:** Requirement "C++ layer routes TTS requests based on voice" — all 3 scenarios

- [ ] **Step 1: Modify BuildRequestUrl to switch endpoint** (5 min)

  将 `ManboTTSProvider.cpp` 的 `BuildRequestUrl` 替换为：

```cpp
std::string ManboTTSProvider::BuildRequestUrl(const TTSRequest& request) const {
    ConfigData config = ConfigManager::Inst()->GetConfig();
    
    if (config.manboVoice == "曼波") {
        // 现有端点
        std::string url = "/apis/mbAIscvip?text=";
        url += UrlEncode(request.text);
        url += "&format=mp3";
        int apiSpeed = config.speechRate * 5;
        url += "&speed=" + std::to_string(apiSpeed);
        url += "&key=" + UrlEncode(config.manboApiKey);
        return url;
    } else {
        // 新端点
        std::string url = "/apis/AIvoice?speaker=";
        url += UrlEncode(config.manboVoice);
        url += "&text=";
        url += UrlEncode(request.text);
        return url;
    }
}
```

- [ ] **Step 2: Verify ParseApiResponse handles new endpoint** (2 min)

  确认 `ParseApiResponse()` 已能正确解析两种端点的返回（它们返回相同格式：`{code: 200, url: "..."}`）。无需修改。

- [ ] **Step 3: Add unit tests** (5 min)

  在 `ManboTTSProviderTests.cpp` 中新增（或创建新文件）：

```cpp
#ifdef RUN_UNIT_TESTS

void TestManboTTSProvider_BuildRequestUrl_DefaultVoice() {
    ManboTTSProvider provider;
    TTSRequest req;
    req.text = "hello";
    
    // 设置默认音色为"曼波"
    ConfigManager::Inst()->SetManboVoice("曼波");
    ConfigManager::Inst()->SetSpeechRate(0);
    ConfigManager::Inst()->SetManboApiKey("test_key");
    
    std::string url = provider.BuildRequestUrl(req);
    assert(url.find("/apis/mbAIscvip") != std::string::npos);
    assert(url.find("key=test_key") != std::string::npos);
    std::cout << "[PASS] TestManboTTSProvider_BuildRequestUrl_DefaultVoice" << std::endl;
}

void TestManboTTSProvider_BuildRequestUrl_OtherVoice() {
    ManboTTSProvider provider;
    TTSRequest req;
    req.text = "hello";
    
    ConfigManager::Inst()->SetManboVoice("顾姐");
    
    std::string url = provider.BuildRequestUrl(req);
    assert(url.find("/apis/AIvoice") != std::string::npos);
    assert(url.find("speaker=%E9%A1%BE%E5%A7%90") != std::string::npos); // URL encoded
    assert(url.find("text=hello") != std::string::npos);
    std::cout << "[PASS] TestManboTTSProvider_BuildRequestUrl_OtherVoice" << std::endl;
    
    // 恢复默认值
    ConfigManager::Inst()->SetManboVoice("曼波");
}

void TestManboTTSProvider_ParseApiResponse_NewEndpoint() {
    ManboTTSProvider provider;
    std::string json = R"({"code":200,"msg":"生成完成!","url":"http://example.com/test.mp3","api_source":"test"})";
    TTSResponse resp = provider.ParseApiResponse(json);
    assert(resp.success == true);
    assert(resp.errorMsg == "http://example.com/test.mp3");
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_NewEndpoint" << std::endl;
}

#endif
```

  在现有测试运行函数（如 `RunAllTests()`）中调用这些测试。

- [ ] **Step 4: Run unit tests** (2 min)

  Run: 编译 Debug 配置并运行测试可执行文件
  Expected: 所有测试输出 `[PASS]`

---

## Task 6: UI 控件 — ConfigWindow.xaml 与 .xaml.cs

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml:235-270`
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs:60,99-105,258,396-409`

**Spec Coverage:** Requirement "UI provides voice selection control" — all 3 scenarios

- [ ] **Step 1: Add ComboBox to ManboConfigPanel in XAML** (3 min)

  在 `ConfigWindow.xaml` 的 `ManboConfigPanel` 中，在 `ManboApiKeyTextBox` 下方添加：

```xml
                                    <StackPanel Orientation="Horizontal" Margin="0,8,0,0">
                                        <Label
                                            Content="音色选择"
                                            VerticalAlignment="Center"
                                            FontSize="14"
                                            Foreground="#222"
                                            Margin="0,0,8,0" />
                                        <ComboBox
                                            Name="ManboVoiceComboBox"
                                            Width="150"
                                            FontSize="14"
                                            SelectionChanged="ManboVoiceComboBox_SelectionChanged">
                                            <ComboBoxItem Content="曼波（付费）" Tag="曼波" />
                                        </ComboBox>
                                    </StackPanel>
```

- [ ] **Step 2: Populate ComboBox with dynamic voices in FillConfig** (3 min)

  在 `ConfigWindow.xaml.cs` 的 `FillConfig()` 方法中，在设置 `ManboApiKeyTextBox` 的代码附近添加：

```csharp
            // 填充音色列表
            ManboVoiceComboBox.Items.Clear();
            ManboVoiceComboBox.Items.Add(new System.Windows.Controls.ComboBoxItem 
            { 
                Content = "曼波（付费）", 
                Tag = "曼波" 
            });
            
            foreach (var voice in ToolsMain.ManboVoiceList)
            {
                if (voice != "曼波")
                {
                    ManboVoiceComboBox.Items.Add(new System.Windows.Controls.ComboBoxItem 
                    { 
                        Content = voice, 
                        Tag = voice 
                    });
                }
            }
            
            // 选择保存的音色
            string savedVoice = config.MANBO_VOICE ?? "曼波";
            bool found = false;
            foreach (System.Windows.Controls.ComboBoxItem item in ManboVoiceComboBox.Items)
            {
                if ((string)item.Tag == savedVoice)
                {
                    ManboVoiceComboBox.SelectedItem = item;
                    found = true;
                    break;
                }
            }
            if (!found && !string.IsNullOrEmpty(savedVoice))
            {
                // 如果保存的音色不在列表中，添加并选中
                var newItem = new System.Windows.Controls.ComboBoxItem 
                { 
                    Content = savedVoice, 
                    Tag = savedVoice 
                };
                ManboVoiceComboBox.Items.Add(newItem);
                ManboVoiceComboBox.SelectedItem = newItem;
            }
```

- [ ] **Step 3: Add SelectionChanged event handler** (2 min)

  在 `ConfigWindow.xaml.cs` 中添加：

```csharp
        private void ManboVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (ManboVoiceComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem item)
            {
                string voice = item.Tag as string;
                if (!string.IsNullOrEmpty(voice))
                {
                    GlobalEventListener.Invoke("ConfigChanged", $"MANBO_VOICE:{voice}");
                }
            }
        }
```

- [ ] **Step 4: Handle ConfigChanged event in ToolsMain** (2 min)

  在 `ToolsMain.cs` 的 `ConfigChanged()` 方法中，找到处理 `MANBO_API_KEY` 的地方，在其下方添加：

```csharp
                ["MANBO_VOICE"] = (k, v) => _Config.Config.MANBO_VOICE = v,
```

- [ ] **Step 5: Build and verify UI** (2 min)

  Run: `msbuild JonysandMHDanmuTools.sln /p:Configuration=Release /p:Platform=x64`
  Expected: 0 errors

---

## Task 7: vcxproj / filters 更新与最终编译验证

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj.filters`

**Spec Coverage:** 构建完整性

- [ ] **Step 1: Check if new test files need project update** (2 min)

  如果在 Task 5 中创建了新的测试文件（如 `ManboTTSProviderVoiceTests.cpp`），需要：
  1. 在 `.vcxproj` 的 `<ClCompile>` 组中按字母顺序添加文件
  2. 在 `.vcxproj.filters` 的 `UnitTests` Filter 下添加
  3. 如果是 Debug-only 测试，添加：`<ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>`

  如果修改了现有 `ManboTTSProviderTests.cpp`，则无需更新项目文件。

- [ ] **Step 2: Full solution build** (3 min)

  Run: `msbuild JonysandMHDanmuTools.sln /p:Configuration=Release /p:Platform=x64 /m`
  Expected: 0 errors, 0 warnings related to our changes

- [ ] **Step 3: Run all unit tests** (2 min)

  Run: 启动 Debug 编译的测试项目（含 `RUN_UNIT_TESTS` 宏）
  Expected: 所有测试输出 `[PASS]`，无失败

- [ ] **Step 4: Git diff review** (2 min)

  Run: `git diff --stat`
  Expected: 确认修改范围正确，无多余改动

---

## Plan Self-Review

**Spec coverage check:**
- [x] C# UI fetch voice list (3 scenarios) → Task 4
- [x] UI voice selection control (3 scenarios) → Task 6
- [x] C++ route TTS by voice (3 scenarios) → Task 5
- [x] Config field sync across layers (2 scenarios) → Tasks 1-3
- [x] Backward compatibility (1 scenario) → Task 1 Step 3 (default value)

**Placeholder scan:**
- [x] 无 TBD/TODO
- [x] 无 "add appropriate error handling" 等模糊描述
- [x] 每步包含完整代码

**Type consistency:**
- [x] `manboVoice` (C++ ConfigData) ↔ `ManboVoice` (C++/CLI proxy) ↔ `MANBO_VOICE` (C# MainConfig) — 命名符合项目规范
- [x] 所有文件路径使用绝对路径或相对于 workspace 的路径
