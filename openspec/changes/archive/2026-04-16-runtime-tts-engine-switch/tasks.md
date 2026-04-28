# 运行时 TTS 引擎切换实现计划

**Goal:** 支持运行时动态切换 TTS 引擎，所有引擎统一队列管理，严格串行播放

**Architecture:** 扩展 AsyncTTSRequest 添加引擎类型，统一队列管理

**Tech Stack:** C++ (TTSManager, ConfigFieldRegistry)

---

## Task 1: TextToSpeech.h 添加方法声明和枚举

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h`

**Step 1: 添加 TTSEngineType 枚举**

在 `AsyncTTSState` 枚举后添加：
```cpp
enum class TTSEngineType {
    Auto,
    MiniMax,
    MiMo,
    SAPI
};
```

**Step 2: 添加 engineType 字段到 AsyncTTSRequest**

在 `AsyncTTSRequest` 结构体中添加：
```cpp
TTSEngineType engineType;
```

**Step 3: 添加方法声明**

在 public 或 private 区添加：
```cpp
// 获取当前配置的引擎类型
TTSEngineType GetActiveEngineType() const;
// SAPI 同步播放（用于队列管理）
bool SpeakWithSapiSync(const TString& text);
// 重新创建TTSProvider（用于运行时切换引擎）
void RefreshTTSProvider();
```

**Step 4: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 2: TextToSpeech.cpp 实现统一队列管理

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

**Step 1: 实现 GetActiveEngineType()**

```cpp
TTSManager::TTSEngineType TTSManager::GetActiveEngineType() const
{
    const auto& config = ConfigManager::Inst()->GetConfig();
    if (config.ttsEngine.empty() || config.ttsEngine == "auto") {
        return TTSEngineType::Auto;
    }
    if (config.ttsEngine == "minimax") {
        return TTSEngineType::MiniMax;
    }
    if (config.ttsEngine == "mimo") {
        return TTSEngineType::MiMo;
    }
    if (config.ttsEngine == "sapi") {
        return TTSEngineType::SAPI;
    }
    return TTSEngineType::Auto;
}
```

**Step 2: 实现 SpeakWithSapiSync()**

基于现有 `SpeakWithSapi()` 修改，使用 SPF_SYNC：
```cpp
bool TTSManager::SpeakWithSapiSync(const TString& text)
{
    std::lock_guard<std::mutex> lock(sapiMutex_);

    if (pVoice == NULL) {
        LOG_ERROR(TEXT("SpeakWithSapiSync: pVoice is NULL"));
        return false;
    }

    // ... 设置语音参数（与 SpeakWithSapi 相同）

    std::wstring ssml = L"<speak version='1.0' xml:lang='zh-CN'><prosody pitch='" + pitchStr + L"'>" + safeText + L"</prosody></speak>";
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_SYNC, NULL);
    return SUCCEEDED(hr);
}
```

**Step 3: 修改 Speak() 方法**

在 `Speak()` 方法中，设置 `engineType` 为当前配置的引擎类型：
```cpp
bool TTSManager::Speak(const TString& text)
{
    AsyncTTSRequest req;
    req.text = text;
    req.engineType = GetActiveEngineType();  // 根据当前配置
    req.state = AsyncTTSState::Pending;

    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    asyncPendingQueue_.push_back(req);
    return true;
}
```

**Step 4: 修改 ProcessPendingRequest() 处理 SAPI**

在函数开头添加判断：
```cpp
void TTSManager::ProcessPendingRequest(std::list<AsyncTTSRequest>::iterator it)
{
    AsyncTTSRequest& req = *it;

    // 处理 SAPI 请求
    if (req.engineType == TTSEngineType::SAPI) {
        LOG_INFO(TEXT("TTS Async: Processing SAPI request"));
        bool success = SpeakWithSapiSync(req.text);
        req.state = success ? AsyncTTSState::Completed : AsyncTTSState::Failed;
        req.errorMessage = success ? "" : "SAPI speak failed";
        return;
    }

    // MiniMax/MiMo 继续走原有 API 流程...
}
```

**Step 5: 实现 RefreshTTSProvider()**

```cpp
void TTSManager::RefreshTTSProvider()
{
    const auto& config = ConfigManager::Inst()->GetConfig();
    LOG_INFO(TEXT("Refreshing TTS provider, engine: %s"), config.ttsEngine.c_str());
    ttsProvider = TTSProviderFactory::Create(
        GetMIMO_API_KEY(),
        GetMINIMAX_API_KEY(),
        config.ttsEngine);
    LOG_INFO(TEXT("TTS provider refreshed successfully"));
}
```

**Step 6: 在构造函数中注册配置监听**

在 TTSManager 构造函数中添加：
```cpp
ConfigManager::Inst()->AddConfigChangedListener([this](const ConfigData& config) {
    this->RefreshTTSProvider();
});
```

**Step 7: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 3: ConfigFieldRegistry.cpp 注册 ttsEngine 回调

**Files:**
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp`

**Step 1: 将 ttsEngine 注册改为使用 REGISTER_FIELD_WITH_CALLBACK**

将：
```cpp
REGISTER_FIELD("ttsEngine", std::string, ttsEngine, ConfigFieldType::String);
```

改为：
```cpp
REGISTER_FIELD_WITH_CALLBACK("ttsEngine", std::string, ttsEngine, ConfigFieldType::String,
    [](ConfigData& cfg) {
        TTSManager::Inst()->RefreshTTSProvider();
    });
```

**Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## 文件清单

| 文件 | 修改类型 |
|------|---------|
| `MonsterOrderWilds/TextToSpeech.h` | 添加 TTSEngineType 枚举、engineType 字段、方法声明 |
| `MonsterOrderWilds/TextToSpeech.cpp` | 实现统一队列管理、SAPI 同步播放、配置监听 |
| `MonsterOrderWilds/ConfigFieldRegistry.cpp` | ttsEngine 注册回调 |