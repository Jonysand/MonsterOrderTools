## Context

Manbo TTS Provider 当前使用旧 API (`api-v2.cenguigui.cn`)，无 API Key 鉴权，无 speed 参数。新 API (`api.milorapart.top`) 需要 API Key 且支持 speed 参数。需替换 API 端点、添加配置字段（参考 idCode 的注册表存储模式）、修改 UI。

## Goals / Non-Goals

**Goals:**
- 替换 Manbo API 端点和响应解析逻辑
- 添加 manboApiKey 配置字段，存储到注册表，全链路参考 idCode 实现
- UI 添加独立的 Manbo API Key 输入区域
- Manbo 请求添加 speed 参数，从全局 speechRate 线性映射

**Non-Goals:**
- 不修改 TTSRequest 结构体
- 不修改其他 Provider（MiMo/SAPI）
- 不改变 TTS 引擎选择和降级逻辑

## Decisions

1. **API Key 存储到注册表** — 参考 idCode 模式，存储到 `HKCU\Software\MonsterOrderWilds\ManboApiKey`。不写入 JSON 配置文件。
2. **仅修改现有 ManboTTSProvider** — 不创建新类，直接修改 BuildRequestUrl、ParseApiResponse、RequestTTS。
3. **Speed 线性映射** — `api_speed = speechRate * 5`，在 BuildRequestUrl 内部完成。
4. **UI 独立 Manbo 设置区域** — 与 MiMo/SAPI 设置区域平级，包含 PasswordBox 输入栏。

## Risks / Trade-offs

- [API Key 泄露风险] → PasswordBox 使用密码输入模式（掩码显示），仅存储注册表
- [旧 API 不可用] → 直接替换端点，无向后兼容；降级链 manbo → mimo → sapi 保持原有逻辑
