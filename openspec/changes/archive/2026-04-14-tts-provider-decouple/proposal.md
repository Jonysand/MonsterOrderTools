## Why

TTSManager 直接持有 MimoTTSClient 指针，违反依赖倒置原则。MiMo 特有概念（dialect/role）残留在 AsyncTTSRequest 结构体中，阻碍多 TTS 引擎扩展。需要通过 ITTSProvider 接口统一管理，实现真正的解耦。

**为什么现在做：** 代码中已存在未使用的 ITTSProvider 接口，但 TTSManager 未使用它，这是技术债务的积累。

## What Changes

1. TTSManager 通过 `std::unique_ptr<ITTSProvider>` 持有 TTS 提供者，不再直接持有 `MimoTTSClient*`
2. 清理 AsyncTTSRequest 结构体中的 dialect、role 残留字段
3. TTSManager 通过接口调用 RequestTTS，MiMo 特有逻辑封装在 XiaomiTTSProvider 内部
4. 删除直接调用 mimoClient 的代码路径

## Capabilities

### Modified Capabilities
- `TTSManager`: 从直接依赖 MimoTTSClient 改为依赖 ITTSProvider 接口

### Removed Capabilities
- `AsyncTTSRequest.dialect`: 字段残留，配置已删除
- `AsyncTTSRequest.role`: 字段残留，配置已删除

## Impact

**受影响的代码：**
- `TextToSpeech.h`: 删除 `MimoTTSClient* mimoClient` 成员，添加 `std::unique_ptr<ITTSProvider> ttsProvider`
- `TextToSpeech.cpp`: 重构 SpeakWithMimoAsync → 使用接口调用
- `AsyncTTSRequest`: 删除 dialect、role 字段
- `ITTSProvider`: 已有接口，可能需要扩展方法
