# Credential TTS Key Split - Proposal

## Why

目前 `credentials.dat` 中 `tts_api_key` 和 `tts_provider` 字段被 MiMo 和 MiniMax 两个 TTS provider 共用。但实际上两个 provider 需要独立的 API key，且它们的回滚机制（fallback）是独立的。需要将这两个字段拆分为各自独立的 API key 字段。

此外，现有的 `tts_provider` 字段用于选择 TTS provider，但实际使用时用户更倾向于"自动"选择（按一定顺序尝试可用 provider），而不是固定选择某一个。

## What Changes

1. **移除字段**：
   - `tts_api_key` (credentials.dat)
   - `tts_provider` (credentials.dat, AI_PROVIDER JSON)

2. **新增字段**：
   - `mimo_tts_api_key` (credentials.dat) - MiMo TTS provider 专用 API key
   - `minimax_tts_api_key` (credentials.dat) - MiniMax TTS provider 专用 API key

3. **回滚逻辑变更**：
   - 当用户选择 `ttsEngine = "auto"` 时，按以下顺序尝试可用 provider：minimax → xiaomi → sapi
   - 当用户明确选择 `mimo` 时，只使用 MiMo TTS
   - 当用户明确选择 `sapi` 时，只使用 SAPI

## Capabilities

### Modified Capabilities

- **TTS Provider Fallback**: 修改自动模式下的回滚顺序为 minimax → xiaomi → sapi

## Risks / Trade-offs

- **迁移风险**: 旧版 credentials.dat 中的 tts_api_key 字段将不再被读取，需要用户重新配置
- **回滚顺序变更**: 原有 auto 模式回滚顺序可能是 xiaomi -> sapi，新顺序 minimax -> xiaomi -> sapi 会改变原有行为。如果用户之前依赖 xiaomi 作为首选 TTS，变更后将优先使用 minimax
- **C# 层修改**: 需要同时修改 DataStructures.cs、Utils.cs、ProxyClasses.cs、ConfigWindow.xaml 以支持 minimaxApiKey 配置字段
- **GetAI_PROVIDER() 变更**: TTS provider 不再由 AI_PROVIDER JSON 控制，需确认 CaptainCheckInModule 是否依赖其中的 TTS 字段

## Impact

### 受影响的文件

| 文件 | 修改内容 |
|------|---------|
| `MonsterOrderWilds/CredentialsManager.cpp` | 读取 `mimo_tts_api_key` 和 `minimax_tts_api_key`，移除 `tts_provider` 和 `tts_api_key` |
| `MonsterOrderWilds/CredentialsManager.h` | 添加 `GetMINIMAX_API_KEY()` 函数声明 |
| `MonsterOrderWilds/TTSProviderFactory.cpp` | 修改回滚逻辑，默认 minimax → xiaomi → sapi |
| `MonsterOrderWilds/TTSProviderTests.cpp` | 更新测试用例以适应新的字段名 |
| `JonysandMHDanmuTools/DataStructures.cs` | 添加 `MinimaxApiKey` 字段 |
| `JonysandMHDanmuTools/Utils.cs` | 注册 `minimaxApiKey` 字段，添加 `MINIMAX_API_KEY` 属性 |
| `JonysandMHDanmuTools/ProxyClasses.cs` | 添加 `MinimaxApiKey` 属性 |
| `JonysandMHDanmuTools/ConfigWindow.xaml` | 添加 MiniMax API Key 输入框 |
| `openspec/specs/tts-provider/spec.md` | 更新字段引用 |