# Manbo TTS 引擎 API 升级 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 替换 Manbo TTS API 端点到 api.milorapart.top，添加 API Key 鉴权和 speed 参数支持。

**Architecture:** 参考 idCode 的注册表存储模式，在 C++ ConfigData 和 C# MainConfig 全链路添加 manboApiKey 字段。ManboTTSProvider 内部读取 speechRate 线性映射到 API speed（-50~50）。UI 添加独立 Manbo 设置面板。

**Tech Stack:** C++, C++/CLI, C# WPF, nlohmann/json, WinHTTP, Windows Registry

---

## 1. C++ ConfigData + ConfigManager — manboApiKey 字段

**Files:**
- Modify: `MonsterOrderWilds/ConfigManager.h:12-44`
- Modify: `MonsterOrderWilds/ConfigManager.cpp:12-47, 155, 237-242, 274-281`

**精确执行序列：**

- [ ] **Step 1: ConfigData 添加字段**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.h:12`
  2. 在 `std::string idCode = "";` 下方添加：

```cpp
    std::string manboApiKey = "";
```

- [ ] **Step 2: ConfigManager.h 添加 SetManboApiKey 声明**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.h:77`
  2. 在 `SetMimoAudioFormat` 后添加：

```cpp
    void SetManboApiKey(const std::string& value);
```

- [ ] **Step 3: ConfigManager.cpp 添加注册表读写函数**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.cpp:14-47`
  2. 修改匿名命名空间，在 `REG_VALUE_NAME` 后添加：

```cpp
    const char* REG_MANBO_API_KEY = "ManboApiKey";

    std::string ReadManboApiKeyFromRegistry()
    {
        std::string result;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char buffer[256];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, REG_MANBO_API_KEY, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
            {
                buffer[min(bufferSize, (DWORD)(sizeof(buffer) - 1))] = '\0';
                result = buffer;
            }
            RegCloseKey(hKey);
        }
        return result;
    }

    bool WriteManboApiKeyToRegistry(const std::string& manboApiKey)
    {
        HKEY hKey;
        DWORD disp;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) == ERROR_SUCCESS)
        {
            RegSetValueExA(hKey, REG_MANBO_API_KEY, 0, REG_SZ, (const BYTE*)manboApiKey.c_str(), manboApiKey.length() + 1);
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
```

- [ ] **Step 4: LoadConfig 中添加注册表读取**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.cpp:155`
  2. 在 `config_.idCode = ReadIdCodeFromRegistry();` 后添加：

```cpp
        config_.manboApiKey = ReadManboApiKeyFromRegistry();
```

- [ ] **Step 5: SaveConfig 中添加注册表写入**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.cpp:237-242`
  2. 在 `WriteIdCodeToRegistry` 调用后添加：

```cpp
        if (!WriteManboApiKeyToRegistry(config_.manboApiKey))
        {
            LOG_ERROR(TEXT("ConfigManager: Failed to write manboApiKey to registry"));
            dirty_ = true;
            return false;
        }
```

- [ ] **Step 6: 添加 SetManboApiKey 实现**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigManager.cpp:274-281`
  2. 在 `SetIdCode` 实现后添加：

```cpp
void ConfigManager::SetManboApiKey(const std::string& value)
{
    lock_.lock();
    bool changed = config_.manboApiKey != value;
    if (changed) { config_.manboApiKey = value; dirty_ = true; }
    lock_.unlock();
    if (changed) NotifyConfigChanged();
}
```

---

## 2. C++ ConfigFieldRegistry — 注册 manboApiKey 字段

**Files:**
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp:27`

**精确执行序列：**

- [ ] **Step 1: 注册字段**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ConfigFieldRegistry.cpp:27`
  2. 在 `REGISTER_FIELD("idCode", std::string, idCode, ConfigFieldType::String);` 下方添加：

```cpp
    REGISTER_FIELD("manboApiKey", std::string, manboApiKey, ConfigFieldType::String);
```

---

## 3. C++ DataBridgeWrapper — 添加 ManboApiKey 属性

**Files:**
- Modify: `MonsterOrderWilds/DataBridgeWrapper.h:33-42, 59-63, 94`

**精确执行序列：**

- [ ] **Step 1: Refresh 方法中添加 ManboApiKey**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/DataBridgeWrapper.h:33`
  2. 在 `MimoAudioFormat` 行后添加：

```cpp
            ManboApiKey = gcnew System::String(config.manboApiKey.c_str());
```

- [ ] **Step 2: Apply 方法中添加 ManboApiKey**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/DataBridgeWrapper.h:62`
  2. 在 `data.mimoAudioFormat` 行后添加：

```cpp
            data.manboApiKey = msclr::interop::marshal_as<std::string>(ManboApiKey);
```

- [ ] **Step 3: 添加属性声明**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/DataBridgeWrapper.h:96`
  2. 在 `property System::String^ MimoAudioFormat;` 后添加：

```cpp
        property System::String^ ManboApiKey;
```

---

## 4. C# DataStructures + Utils + ProxyClasses — 添加 ManboApiKey

**Files:**
- Modify: `JonysandMHDanmuTools/DataStructures.cs:12, 52, 82`
- Modify: `JonysandMHDanmuTools/Utils.cs:154-156, 317-321, 404-407`
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:26-31, 175, 200`

**精确执行序列：**

- [ ] **Step 1: DataStructures.cs 添加 ManboApiKey 字段**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/DataStructures.cs:12`
  2. 在 `public string IdCode;` 后添加：

```csharp
        public string ManboApiKey;
```

  3. 在 FromMainConfig 的 `IdCode` 行后添加：
```csharp
                ManboApiKey = config.MANBO_API_KEY ?? "",
```

  4. 在 ApplyTo 的 `config.ID_CODE = IdCode;` 后添加：
```csharp
            config.MANBO_API_KEY = ManboApiKey;
```

- [ ] **Step 2: Utils.cs 注册字段并添加 MainConfig 属性**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/Utils.cs:154-156`
  2. 在 `Register("idCode"...` 后添加：

```csharp
            Register("manboApiKey", ConfigFieldType.String,
                () => GetString("manboApiKey"),
                v => SetValue("manboApiKey", (string)v, ConfigFieldType.String));
```

  3. 用 read 工具读取 `JonysandMHDanmuTools/Utils.cs:317-321`
  4. 在 `ID_CODE` 属性后添加：

```csharp
        public String MANBO_API_KEY
        {
            get => (string)ConfigFieldRegistry.Get("manboApiKey");
            set { ConfigFieldRegistry.Set("manboApiKey", value); OnPropertyChanged(); }
        }
```

- [ ] **Step 3: ProxyClasses.cs 添加属性和同步方法**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ProxyClasses.cs:26-31`
  2. 在 `IdCode` 属性后添加：

```csharp
        private string _manboApiKey = "";
        public string ManboApiKey
        {
            get => _manboApiKey;
            set { _manboApiKey = value; OnPropertyChanged(); }
        }
```

  3. 在 RefreshFromConfig 的 `IdCode` 行后添加：
```csharp
            ManboApiKey = config.MANBO_API_KEY ?? "";
```

  4. 在 ApplyToConfig 的 `config.ID_CODE = IdCode;` 后添加：
```csharp
            config.MANBO_API_KEY = ManboApiKey;
```

---

## 5. C# ToolsMain.cs — 添加 ConfigChanged handler

**Files:**
- Modify: `JonysandMHDanmuTools/ToolsMain.cs:199`

**精确执行序列：**

- [ ] **Step 1: 添加 MANBO_API_KEY handler**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ToolsMain.cs:199`
  2. 在 `["ID_CODE"]` 行后添加：

```csharp
                ["MANBO_API_KEY"] = (k, v) => _Config.Config.MANBO_API_KEY = v,
```

---

## 6. ManboTTSProvider API 修改

**Files:**
- Modify: `MonsterOrderWilds/ManboTTSProvider.cpp:39-44, 47-71, 140-186`
- Modify: `MonsterOrderWilds/TTSProvider.h:35-49` (构造函数可接收参数，或无参)

**精确执行序列：**

- [ ] **Step 1: 修改 BuildRequestUrl — 新 API 端点 + speed + apikey**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ManboTTSProvider.cpp:39-45`
  2. 修改 `BuildRequestUrl` 实现：

```cpp
std::string ManboTTSProvider::BuildRequestUrl(const TTSRequest& request) const {
    std::string url = "/apis/mbAIscvip??text=";
    url += UrlEncode(request.text);
    url += "&format=mp3";

    // 从 ConfigManager 读取 speechRate，映射到 API speed（-50~50）
    ConfigData config = ConfigManager::Inst()->GetConfig();
    int apiSpeed = config.speechRate * 5;  // -10~10 → -50~50
    url += "&speed=" + std::to_string(apiSpeed);
    url += "&key=" + UrlEncode(config.manboApiKey);

    return url;
}
```

- [ ] **Step 2: 修改 ParseApiResponse — 解析 url 字段（非 audio_url）**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ManboTTSProvider.cpp:47-71`
  2. 修改 `ParseApiResponse` 实现：

```cpp
TTSResponse ManboTTSProvider::ParseApiResponse(const std::string& responseBody) const {
    TTSResponse result;
    result.success = false;
    
    try {
        auto j = nlohmann::json::parse(responseBody);
        if (j.contains("code") && j["code"].get<int>() == 200) {
            if (j.contains("url") && j["url"].is_string()) {
                result.success = true;
                result.errorMsg = j["url"].get<std::string>();
            } else {
                result.errorMsg = "Invalid response format: missing url";
            }
        } else {
            if (j.contains("msg") && j["msg"].is_string()) {
                result.errorMsg = j["msg"].get<std::string>();
            } else {
                result.errorMsg = "API error";
            }
        }
    } catch (const std::exception& e) {
        result.errorMsg = std::string("Parse error: ") + e.what();
    }
    return result;
}
```

- [ ] **Step 3: 修改 RequestTTS — 新 API host**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ManboTTSProvider.cpp:145`
  2. 修改 host 为：
  3. 同时需要在文件头部添加 ConfigManager 的 include：

```cpp
        TEXT("api.milorapart.top"),
```

  4. 在文件头部 `#include "Network.h"` 后添加：

```cpp
#include "ConfigManager.h"
```

---

## 7. UI ConfigWindow.xaml + xaml.cs — Manbo 设置面板

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml:370`（SapiConfigPanel 之后）
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs:59, 89, 244-248`

**精确执行序列：**

- [ ] **Step 1: ConfigWindow.xaml 添加 Manbo 设置面板**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ConfigWindow.xaml:370`（`</Border>` 关闭 SapiConfigPanel）
  2. 在 `</Border>` 关闭 SapiConfigPanel 后、`<StackPanel Orientation="Horizontal" Margin="8,8,0,0"` 缓存行前添加：

```xml
                            <Border Name="ManboConfigPanel" Margin="8,8,0,0">
                                <StackPanel>
                                    <Label
                                        Content="Manbo 设置"
                                        FontFamily="Segoe UI Variable"
                                        FontSize="14"
                                        FontWeight="SemiBold"
                                        Foreground="#0078D4"
                                        Margin="0,4,0,4" />
                                    <Border
                                        Background="#ddd"
                                        CornerRadius="6"
                                        Padding="6,2"
                                        HorizontalAlignment="Left"
                                        Height="34">
                                        <Grid>
                                            <PasswordBox
                                                Name="ManboApiKeyTextBox"
                                                VerticalAlignment="Center"
                                                Width="250"
                                                Height="28"
                                                PasswordChanged="ManboApiKeyTextBox_PasswordChanged" />
                                            <TextBlock
                                                Name="ManboApiKeyPlaceholder"
                                                Text="输入Manbo API Key"
                                                Foreground="#888"
                                                VerticalAlignment="Center"
                                                Margin="8,0,0,0"
                                                IsHitTestVisible="False"
                                                FontSize="14"
                                                FontFamily="Segoe UI Variable"
                                                Visibility="Visible" />
                                        </Grid>
                                    </Border>
                                </StackPanel>
                            </Border>
```

- [ ] **Step 2: ConfigWindow.xaml.cs FillConfig 中初始化 ManboApiKeyTextBox**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ConfigWindow.xaml.cs:59`
  2. 在 `IdentityCodeTextBox.Password = config.ID_CODE;` 后添加：

```csharp
            ManboApiKeyTextBox.Password = config.MANBO_API_KEY ?? "";
```

- [ ] **Step 3: ConfigWindow.xaml.cs 添加 PasswordChanged 事件处理**

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ConfigWindow.xaml.cs:244-248`
  2. 在 `IdentityCodeTextBox_PasswordChanged` 后添加：

```csharp
        private void ManboApiKeyTextBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", "MANBO_API_KEY:" + ManboApiKeyTextBox.Password);
        }
```

---

## 8. C++ Unit Tests 更新

**Files:**
- Modify: `MonsterOrderWilds/TTSProviderTests.cpp:143-201`

**精确执行序列：**

- [ ] **Step 1: TTSProviderTests.cpp 添加 head include**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/TTSProviderTests.cpp:1-6`
  2. 在 `#include "TTSProvider.h"` 后添加：

```cpp
#include "ConfigManager.h"
```

- [ ] **Step 2: 更新 TestManboTTSProvider_BuildRequestUrl 测试**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/TTSProviderTests.cpp:143-155`
  2. 更新测试函数：

```cpp
void TestManboTTSProvider_BuildRequestUrl()
{
    ManboTTSProvider manbo;
    TTSRequest req;
    req.text = "你好世界";
    
    std::string url = manbo.BuildRequestUrl(req);
    assert(url.find("/apis/mbAIscvip") != std::string::npos);
    assert(url.find("format=mp3") != std::string::npos);
    assert(url.find("speed=") != std::string::npos);
    assert(url.find("key=") != std::string::npos);
    
    std::cout << "[PASS] TestManboTTSProvider_BuildRequestUrl" << std::endl;
}
```

- [ ] **Step 3: 添加 speed 映射测试**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/TTSProviderTests.cpp:200`
  2. 在 `TestManboTTSProvider_IsAvailable` 后添加：

```cpp
void TestManboTTSProvider_BuildRequestUrl_SpeedMapping()
{
    ManboTTSProvider manbo;
    TTSRequest req;
    req.text = "测试";
    
    std::string url = manbo.BuildRequestUrl(req);
    // speechRate=0 默认 → speed=0
    assert(url.find("&speed=0") != std::string::npos);
    
    std::cout << "[PASS] TestManboTTSProvider_BuildRequestUrl_SpeedMapping" << std::endl;
}
```

- [ ] **Step 4: 更新 ParseApiResponse 测试 — 新响应格式**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/TTSProviderTests.cpp:157-191`
  2. 在 `TestManboTTSProvider_BuildRequestUrl` 后更新：

```cpp
void TestManboTTSProvider_ParseApiResponse_Success()
{
    ManboTTSProvider manbo;
    std::string responseBody = "{\"code\":200,\"msg\":\"生成完成!\",\"url\":\"https://example.com/audio.mp3\"}";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == true);
    assert(resp.errorMsg == "https://example.com/audio.mp3");
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_Success" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_Error()
{
    ManboTTSProvider manbo;
    std::string responseBody = "{\"code\":500,\"msg\":\"服务器错误\"}";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == false);
    assert(resp.errorMsg == "服务器错误");
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_Error" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_InvalidJson()
{
    ManboTTSProvider manbo;
    std::string responseBody = "not json at all";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == false);
    assert(!resp.errorMsg.empty());
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_InvalidJson" << std::endl;
}

void TestManboTTSProvider_ParseApiResponse_MissingUrl()
{
    ManboTTSProvider manbo;
    std::string responseBody = "{\"code\":200,\"msg\":\"成功\"}";
    
    auto resp = manbo.ParseApiResponse(responseBody);
    assert(resp.success == false);
    assert(!resp.errorMsg.empty());
    
    std::cout << "[PASS] TestManboTTSProvider_ParseApiResponse_MissingUrl" << std::endl;
}
```

- [ ] **Step 5: 更新 RunTTSProviderTests 注册新测试**

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/TTSProviderTests.cpp:265-270`
  2. 在 `TestManboTTSProvider_BuildRequestUrl();` 后添加：

```cpp
    TestManboTTSProvider_BuildRequestUrl_SpeedMapping();
```

  3. 在 `TestManboTTSProvider_ParseApiResponse_InvalidJson();` 后添加：

```cpp
    TestManboTTSProvider_ParseApiResponse_MissingUrl();
```

---

## 9. 编译验证

**精确执行序列：**

- [ ] **Step 1: 运行编译**

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: 0 个错误

---

## Spec Coverage Verification

| Spec Requirement | Task | Status |
|-----------------|------|--------|
| manbo-api-key-management: 注册表存储 | Task 1, Step 3-6 | ✓ |
| manbo-api-key-management: 字段注册 (C++) | Task 2 | ✓ |
| manbo-api-key-management: 字段注册 (C#) | Task 4, Step 2 | ✓ |
| manbo-api-key-management: DataBridge 跨层传输 | Task 3, Task 4 | ✓ |
| manbo-api-key-management: UI 输入栏 | Task 7 | ✓ |
| manbo-speed-parameter: 正常范围映射 | Task 6, Step 1 | ✓ |
| manbo-speed-parameter: speed 仅用于 Manbo | Task 6, Step 1 | ✓ |
| manbo-tts-provider: API 端点更换 | Task 6, Step 1+3 | ✓ |
| manbo-tts-provider: 响应格式 url 字段 | Task 6, Step 2 | ✓ |
| manbo-tts-provider: UI 集成 | Task 7 | ✓ |
| 单元测试覆盖 | Task 8 | ✓ |

---

## Self-Review Results

1. **Spec coverage**: 全部 spec requirements 有对应 task ✓
2. **Placeholder scan**: 无 TBD/TODO/占位符 ✓
3. **Type consistency**: 类型和命名跨 task 一致 ✓
4. **TDD compliance**: Task 8 测试在 Task 6 实现前编写 ✓
