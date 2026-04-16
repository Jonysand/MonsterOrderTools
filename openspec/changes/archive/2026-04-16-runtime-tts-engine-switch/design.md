# 运行时 TTS 引擎切换设计

## Context

当前 TTS 引擎在 `TTSManager` 构造时创建一次，之后不会更新。当用户在 UI 上切换 TTS 引擎后，新配置只被保存到文件，但 `ttsProvider` 仍使用旧的。

## Goals / Non-Goals

**Goals:**
- 支持运行时动态切换 TTS 引擎
- 切换后新的 TTS 请求使用新引擎
- 不影响正在播放的音频
- **所有引擎（MiniMax/MiMo/SAPI）统一队列管理，严格串行播放**

**Non-Goals:**
- 不处理当前正在排队的 TTS 请求的切换（让它自然完成）
- 不修改 MiniMax/MiMo/SAPI Provider 的内部逻辑

## Decisions

### 1. 实现 RefreshTTSProvider()

在 `TTSManager` 中添加 `RefreshTTSProvider()` 方法：

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

### 2. 注册配置变更监听

在 `TTSManager` 构造函数中，向 `ConfigManager` 注册配置变更监听：

```cpp
ConfigManager::Inst()->AddConfigChangedListener([this](const ConfigData& config) {
    this->RefreshTTSProvider();
});
```

### 3. ttsEngine 字段注册回调

在 `ConfigFieldRegistry.cpp` 中，为 `ttsEngine` 字段注册变更回调：

```cpp
REGISTER_FIELD_WITH_CALLBACK("ttsEngine", std::string, ttsEngine, ConfigFieldType::String,
    [](ConfigData& cfg) {
        TTSManager::Inst()->RefreshTTSProvider();
    });
```

注意：`REGISTER_FIELD_WITH_CALLBACK` 在字段值通过 `SetValueByMeta` 更新时会被调用。

### 4. 统一队列管理

所有 TTS 请求（包括 SAPI）都通过 `asyncPendingQueue_` 队列管理，确保严格串行播放。

#### 4.1 扩展 AsyncTTSRequest 添加引擎类型

在 `AsyncTTSRequest` 结构体中添加 `engineType` 字段：

```cpp
enum class TTSEngineType {
    Auto,   // 由 GetActiveEngine() 决定
    MiniMax,
    MiMo,
    SAPI
};

struct AsyncTTSRequest {
    TString text;
    AsyncTTSState state;
    TTSEngineType engineType;  // 新增：指定使用哪个引擎
    // ... 其他字段
};
```

#### 4.2 修改 Speak 方法

修改 `Speak()` 方法，根据当前配置的引擎类型设置 `engineType`：

```cpp
bool TTSManager::Speak(const TString& text)
{
    TTSEngineType type = GetActiveEngineType();  // 根据 config.ttsEngine 返回

    AsyncTTSRequest req;
    req.text = text;
    req.engineType = type;
    req.state = AsyncTTSState::Pending;

    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    asyncPendingQueue_.push_back(req);
    return true;
}
```

#### 4.3 修改 ProcessPendingRequest 处理 SAPI

在 `ProcessPendingRequest` 中判断 `engineType`：

```cpp
void TTSManager::ProcessPendingRequest(std::list<AsyncTTSRequest>::iterator it)
{
    AsyncTTSRequest& req = *it;

    if (req.engineType == TTSEngineType::SAPI) {
        // SAPI 同步播放
        bool success = SpeakWithSapiSync(req.text);
        req.state = success ? AsyncTTSState::Completed : AsyncTTSState::Failed;
        return;
    }

    // MiniMax/MiMo 通过 API 处理
    // ... 原有逻辑
}
```

#### 4.4 SpeakWithSapi 改为同步版本

创建一个 `SpeakWithSapiSync` 方法，等待播放完成：

```cpp
bool TTSManager::SpeakWithSapiSync(const TString& text)
{
    // ... 设置语音参数
    // 使用 SPF_SYNC 而不是 SPF_ASYNC
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_SYNC, NULL);
    return SUCCEEDED(hr);
}
```

## Files to Modify

| File | Changes |
|------|---------|
| `MonsterOrderWilds/TextToSpeech.h` | 添加 `RefreshTTSProvider()`、`GetActiveEngineType()`、`SpeakWithSapiSync()` |
| `MonsterOrderWilds/TextToSpeech.cpp` | 实现统一队列管理、SAPI 同步播放 |
| `MonsterOrderWilds/ConfigFieldRegistry.cpp` | 为 `ttsEngine` 字段注册回调 |

## 流程图

```
用户切换 TTS 引擎
       ↓
UI 调用 GlobalEventListener.Invoke("ConfigChanged", "TTS_ENGINE:sapi")
       ↓
ToolsMain.ConfigChanged 接收消息
       ↓
_Config.Config.TTS_ENGINE = "sapi"
_Config.SaveConfig()  → NativeImports.Config_Save()
       ↓
C++: Config_SetValue("ttsEngine", "sapi", CONFIG_TYPE_STRING)
       ↓
ConfigManager::SetValueByMeta 更新 config_.ttsEngine
       ↓
ConfigFieldRegistry::InvokeOnChanged 调用回调
       ↓
TTSManager::RefreshTTSProvider() 被调用
       ↓
ttsProvider 重新创建（SAPI Provider）
       ↓
新请求进入队列，engineType = SAPI
       ↓
ProcessPendingRequest 识别 SAPI 类型
       ↓
SpeakWithSapiSync 同步播放
```

## 切换行为

| 场景 | 当前队列 | 新请求 |
|------|---------|--------|
| MiniMax → MiMo | MiniMax 播放完 | MiMo |
| MiniMax → SAPI | MiniMax 播放完 | SAPI |
| SAPI → MiniMax | SAPI 播放完 | MiniMax |

所有引擎类型统一队列，串行播放。