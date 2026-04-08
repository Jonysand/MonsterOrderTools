## Context

本次修复解决了以下问题：

1. **连接崩溃问题**：点击 Connect 按钮后 `std::mutex::unlock()` 崩溃
2. **网络请求失败**：WinHTTP 错误 12019 (ERROR_WINHTTP_INCORRECT_HANDLE_STATE)
3. **SAPI 无声音**：默认音量值为 0（静音）
4. **MiMo TTS 无法使用**：读取了错误的 API key 字段名
5. **Chat/TTS Provider 配置缺失**：GetAI_PROVIDER() 未正确构建 JSON

## Decisions

### Decision 1: 修复 Network.cpp std::mutex 崩溃

**问题**：WinHttp callback 线程和 cleanup 线程存在竞态条件，访问已删除的 `HttpsAsyncContext`

**解决方案**：使用 `shared_ptr`/`weak_ptr` 模式管理 `HttpsAsyncContext` 生命周期

```cpp
struct HttpsAsyncContext {
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    std::string response;
    std::atomic<DWORD> error{0};
    Network::HttpsAsyncCallback callback;
    std::mutex mtx;
    std::weak_ptr<std::atomic<bool>> cleanupFlag;  // 使用 weak_ptr
};
```

回调函数先检查 `cleanupFlag.lock()` 是否有效，再访问上下文。

### Decision 2: 移除 WINHTTP_FLAG_ASYNC

**问题**：使用 `WINHTTP_FLAG_ASYNC` 模式但调用顺序错误，导致错误 12019

**解决方案**：移除异步模式标志，使用同步 WinHTTP

```cpp
// 修改前
hSession = WinHttpOpen(..., WINHTTP_FLAG_ASYNC);

// 修改后
hSession = WinHttpOpen(..., 0);
```

### Decision 3: SAPI 音量默认值修复

**问题**：`speechVolume` 默认值为 0，导致 SAPI 无声音

**解决方案**：将默认值从 0 改为 50

| 文件 | 修改 |
|------|------|
| `ConfigManager.h:16` | `speechVolume = 0` → `50` |
| `ConfigWindow.xaml:369` | `Value="80"` → `50` |
| `ProxyClasses.cs:61` | `_speechVolume = 80` → `50` |

### Decision 4: MiMo TTS API Key 字段统一

**问题**：`MimoTTSClient` 使用 `MIMO_API_KEY` 字段，但 credentials.dat 中只有 `tts_api_key`

**解决方案**：修改 `CredentialsManager.cpp` 读取 `tts_api_key` 字段

```cpp
// 修改前
s_mimoApiKey = json.value("MIMO_API_KEY", "");

// 修改后
s_mimoApiKey = json.value("tts_api_key", "");
```

### Decision 5: GetAI_PROVIDER() 自动构建 JSON

**问题**：`GetAI_PROVIDER()` 返回空，CaptainCheckInModule 无法创建 Chat/TTS Provider

**解决方案**：自动从 `tts_provider`、`tts_api_key`、`chat_provider`、`chat_api_key` 构建 JSON

```cpp
nlohmann::json aiProviderJson;
aiProviderJson["tts_provider"] = json.value("tts_provider", "xiaomi");
aiProviderJson["tts_api_key"] = json.value("tts_api_key", "");
aiProviderJson["chat_provider"] = json.value("chat_provider", "deepseek");
aiProviderJson["chat_api_key"] = json.value("chat_api_key", "");
s_aiProvider = aiProviderJson.dump();
```

## 文件修改清单

| 文件 | 修改内容 |
|------|---------|
| `MonsterOrderWilds/Network.cpp` | shared_ptr/weak_ptr 模式，移除 WINHTTP_FLAG_ASYNC |
| `MonsterOrderWilds/TextToSpeech.cpp` | 添加中文语音选择调试日志 |
| `MonsterOrderWilds/ConfigManager.h` | speechVolume 默认值 0→50 |
| `MonsterOrderWilds/CredentialsManager.cpp` | tts_api_key 读取，AI_PROVIDER JSON 构建 |
| `JonysandMHDanmuTools/ConfigWindow.xaml` | 音量滑块默认值 80→50 |
| `JonysandMHDanmuTools/ProxyClasses.cs` | _speechVolume 默认值 80→50 |

## Credentials.dat 字段说明

credentials.dat 中的 API Key 字段：

| 字段 | 用途 |
|------|------|
| `tts_provider` | TTS 提供商 (xiaomi/minimax) |
| `tts_api_key` | TTS API Key (MiMo、Xiaomi、MiniMax 共用) |
| `chat_provider` | Chat AI 提供商 (deepseek/minimax) |
| `chat_api_key` | Chat API Key |
