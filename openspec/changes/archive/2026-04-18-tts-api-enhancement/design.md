## Context

当前 MiniMaxTTSProvider::BuildRequestBody 中的 API 参数与官方要求不一致：
- voice_setting 缺少 text_normalization
- audio_setting 的 sample_rate(32000) 和 bitrate(128000) 需要调整为 22050 和 64000
- 需要移除可能存在的 pronunciation_dict

Mimo TTS（XiaomiTTSProvider）需要支持弹幕文本中的 #标签 语法，将其转换为 <style> 标签。

## Goals / Non-Goals

**Goals:**
- MiniMax API 参数标准化（text_normalization、audio_setting 规格化）
- Mimo TTS 支持 #标签 转 <style> 标签
- 确保音色变更即时生效

**Non-Goals:**
- 不修改 TTS API 密钥获取方式
- 不修改音频播放逻辑
- 不修改配置 UI

## Decisions

### Decision 1: MiniMax API 参数修改

**选择：**
直接修改 MiniMaxTTSProvider::BuildRequestBody 中的 JSON 构建逻辑

**理由：**
- 修改范围明确，仅涉及 BuildRequestBody 方法
- 不影响现有网络请求和响应处理流程

### Decision 2: Mimo #标签 替换

**选择：**
在 XiaomiTTSProvider::BuildRequestBody 中，调用配置中的 mimoStyle 之前，先对 request.text 进行预处理

**理由：**
- 在 BuildRequestBody 中处理可以保持原有配置逻辑的扩展性
- 正则匹配替换实现简单高效

**替换规则：**
- 匹配成对的 #...# 标签（#后面和#前面不能有#）
- 替换为 <style>...</style>

### Decision 3: 音色即时生效

**选择：**
检查 TTSManager/TTSProviderFactory 中 provider 的创建和缓存机制

**理由：**
- 确保用户修改音色配置后不需要重启应用

## Risks / Trade-offs

- **风险**: #标签 嵌套或转义情况未覆盖
  - **缓解**: 当前需求仅支持简单的成对标签，不考虑嵌套场景

- **风险**: Mimo API 对 <style> 标签格式有特殊要求
  - **缓解**: 当前实现中 style 标签直接拼接在文本前，符合 Mimo API 规范
