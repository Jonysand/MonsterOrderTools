## Context

当前 TTS 播放架构中，`ProcessPendingRequest` 处理 SAPI 请求时：
1. 在后台线程调用 `SpeakWithSapiSync` 同步播放
2. 播放完成后标记请求为 Completed
3. 气泡回调在播放前触发

问题：后台线程播放不经过 `audioPlayer` 队列系统，导致：
- SAPI 播放期间，下一个 TTS 请求可能被处理（如果队列已空）
- 多个 SAPI 请求可能同时播放
- 播放状态无法被 `ProcessPlayingState` 管理

## Goals / Non-Goals

**Goals:**
- 确保 SAPI 播放与 MiniMax/MiMo 同样串行播放
- SAPI 播放期间，队列中的下一个请求必须等待
- 不破坏现有 TTS 请求的串行播放保证

**Non-Goals:**
- 不改变 MiniMax/MiMo 的现有播放流程
- 不修改 TTS 引擎选择和降级逻辑
- 不改变气泡显示逻辑

## Decisions

### Decision 1: SAPI 异步播放 + SPEVENT 监听

**选择**: 使用 `ISpVoice::Speak` 的 `SPF_ASYNC` 标志异步播放，通过 `SetNotifyCallbackFunction` 设置播放完成回调

**理由**:
- 不需要生成临时文件
- 复用现有的队列状态机
- SAPI 自带的事件机制可以监听播放完成

**替代方案**:
- 临时文件 + AudioPlayer
  - 缺点：需要磁盘 I/O，临时文件管理复杂
- 内存流捕获 SAPI 输出
  - 缺点：需要转换格式，复杂度高

### Decision 2: SAPI 请求处理流程

**选择**: SAPI 请求作为特殊请求处理：
1. `ProcessPendingRequest`: 发起 `SPF_ASYNC` 播放请求，设置播放完成回调，state=Playing
2. `ProcessPlayingState`: 等待 SPEVENT 回调触发播放完成
3. 回调中标记 Completed，并触发下一个请求处理

**理由**:
- 不需要临时文件
- 播放完成事件驱动下一个请求处理
- 状态流转与 MiniMax/MiMo 类似

## 实现方案

### TextToSpeech.h 修改

```cpp
// 添加成员变量
static void WINAPI SapiSpeakCallback(WORD wNotify, WORD wParam, LPVOID pData, LPVOID pUserData);
bool ProcessSapiSpeakComplete();
```

### TextToSpeech.cpp 修改

```cpp
// ProcessPendingRequest 中 SAPI 处理：
if (req.engineType == TTSEngineType::SAPI) {
    // 1. 气泡回调先触发（在主线程）
    if (req.isCheckinTTS && !req.checkinUsername.empty()) {
        std::wstring usernameW = Utf8ToWstring(req.checkinUsername);
        if (g_checkinTTSPlayCallback) {
            g_checkinTTSPlayCallback(usernameW.c_str(), req.text.c_str(), g_checkinTTSPlayUserData);
        }
    }

    // 2. 异步播放 SAPI
    req.state = AsyncTTSState::Playing;
    req.playbackStarted = true;

    // 使用 SPF_ASYNC 异步播放
    std::wstring ssml = BuildSsml(req.text);
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL);

    if (SUCCEEDED(hr)) {
        // 设置完成回调
        pVoice->SetNotifyCallbackFunction(SapiSpeakCallback, 0, SPFEI(SPEI_SENTENCE_SKIP) | SPFEI(SPEI_TTS_BOOKMARK) | SPFEI(SPEI_STREAM_STARTED) | SPFEI(SPEI_STREAM_ENDED));
    } else {
        req.state = AsyncTTSState::Failed;
        req.errorMessage = "SAPI speak failed";
    }
    return;
}

// SAPI 播放完成回调（全局静态或 member）
void CALLBACK TTSManager::SapiSpeakCallback(WORD wNotify, WORD wParam, LPVOID pData, LPVOID pUserData) {
    if (wNotify == SPEI_STREAM_ENDED) {
        // 通知 TTSManager 主线程处理完成
        PostMessage(hMainWnd, WM_TTS_SAPI_COMPLETE, 0, 0);
    }
}
```

### 关键点

1. **串行保证**: SAPI 使用 `SPF_ASYNC` 播放，但需要等待 `SPEI_STREAM_ENDED` 回调才处理下一个请求
2. **回调机制**: 通过 `SetNotifyCallbackFunction` 设置回调，回调中 PostMessage 通知主线程
3. **主线程处理**: 主线程收到消息后，标记当前请求完成，触发 `ProcessAsyncTTS` 处理下一个

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| SPEVENT 回调不在主线程 | 使用 PostMessage 切换到主线程 |
| SAPI 播放失败未触发回调 | 设置超时机制兜底 |

## Open Questions

1. 是否需要处理 `SPEI_TTS_BOOKMARK` 事件来精确控制播放完成时机？
2. SAPI 播放的超时时间设置为多少合理？