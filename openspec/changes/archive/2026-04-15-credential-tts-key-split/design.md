# Credential TTS Key Split - Design

## Context

目前 `credentials.dat` 中使用 `tts_api_key` 和 `tts_provider` 两个字段供所有 TTS provider 共用。但实际上：
- MiMo TTS 使用 XiaomiTTSProvider
- MiniMax TTS 使用 MiniMaxTTSProvider
- 两个 provider 需要各自独立的 API key

此外，现有的回滚逻辑依赖于 `tts_provider` 字段选择 provider，但在 AUTO 模式下应该按 minimax → xiaomi → sapi 的顺序尝试。

## Goals / Non-Goals

**Goals:**
- 支持 MiMo 和 MiniMax TTS provider 独立的 API key
- 简化 credentials.dat 中的 TTS 相关字段
- AUTO 模式下按 minimax → xiaomi → sapi 的顺序回滚

**Non-Goals:**
- 不修改 ConfigManager 中的配置 JSON 结构（配置层仍使用 mimoApiKey）
- 不修改 ttsEngine 字段的行为（仍支持 auto/mimo/sapi）
- 不改变 TTS provider 的内部实现

## Decisions

### Decision 1: 拆分 TTS API Key 字段

**问题**: 目前 tts_api_key 被多个 provider 共用

**解决方案**: 将 TTS API key 拆分为独立字段

- `mimo_tts_api_key`: Xiaomi/MiMo TTS provider 专用
- `minimax_tts_api_key`: MiniMax TTS provider 专用
- 移除 `tts_provider` 和 `tts_api_key` 字段

**理由**: 两个 provider 需要不同的 API key，拆分后更清晰

### Decision 2: TTSProviderFactory 回滚逻辑

**问题**: AUTO 模式下的回滚顺序

**解决方案**: `TTSProviderFactory::Create()` 接收两个 API key 参数

```cpp
std::unique_ptr<ITTSProvider> TTSProviderFactory::Create(
    const std::string& mimoApiKey,
    const std::string& minimaxApiKey,
    const std::string& ttsEngine);
```

回滚逻辑：
- ttsEngine = "auto":
  - minimaxApiKey 非空 → MiniMaxTTSProvider
  - mimoApiKey 非空 → XiaomiTTSProvider
  - 否则 → SapiTTSProvider
- ttsEngine = "mimo": XiaomiTTSProvider (无论 minimax 是否可用)
- ttsEngine = "sapi": SapiTTSProvider

**理由**: 用户明确选择时尊重选择，自动模式下按新顺序尝试

### Decision 3: GetAI_PROVIDER() 不再包含 TTS 相关字段

**问题**: AI_PROVIDER JSON 中仍包含 tts_provider 和 tts_api_key

**解决方案**: 移除 AI_PROVIDER 中的 TTS 字段

**理由**: TTS provider 不再由 AI_PROVIDER JSON 控制，改为由 TextToSpeech 模块通过 CredentialsManager 直接获取各自 API key

## Risks / Trade-offs

- **迁移风险**: 旧版 credentials.dat 中的 tts_api_key 字段将不再被读取，需要用户重新配置
- **兼容性**: 需要修改 C# 层以支持 minimaxApiKey 配置字段。根据 AGENTS.md 配置字段扩展规则，需要同时修改：DataStructures.cs（添加 MinimaxApiKey 字段）、Utils.cs（注册 minimaxApiKey 字段和添加 MINIMAX_API_KEY 属性）、ProxyClasses.cs（添加 MinimaxApiKey 属性）、ConfigWindow.xaml（添加 MiniMax API Key 输入框）。

## Files to Modify

| File | Changes |
|------|---------|
| `CredentialsManager.cpp` | 读取 mimo_tts_api_key 和 minimax_tts_api_key；添加 GetMINIMAX_API_KEY() |
| `CredentialsManager.h` | 添加 GetMINIMAX_API_KEY() 声明 |
| `TTSProviderFactory.cpp` | 修改 Create() 签名和回滚逻辑 |
| `TTSProvider.h` | 可选：更新 Create() 声明 |
| `TextToSpeech.cpp` | 传递两个 API key 给 TTSProviderFactory |
| `TTSProviderTests.cpp` | 更新测试用例 |