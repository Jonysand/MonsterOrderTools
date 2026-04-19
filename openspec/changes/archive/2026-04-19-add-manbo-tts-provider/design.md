## Context
当前系统已有三种 TTS Provider：
- **MiniMax**：通过 HTTP POST 发送 JSON 请求，返回 hex 编码的音频数据
- **小米 MiMo**：通过 HTTP POST 发送 JSON 请求，返回 base64 编码的音频数据
- **Windows SAPI**：本地语音合成，无需网络

所有 Provider 都实现 `ITTSProvider` 接口，通过 `TTSProviderFactory` 创建。

## Goals / Non-Goals
**Goals:**
- 实现 Manbo TTS Provider，支持通过 HTTP GET 调用 API
- 正确解析 API 返回的 JSON，提取 audio_url
- 从 audio_url 下载 WAV 格式音频数据
- 集成到现有的 TTS 引擎选择和工厂模式
- 实现 Provider 降级策略（manbo → minimax → mimo → sapi）
- 在 UI 中实时显示当前实际使用的 TTS Provider
- 添加单元测试覆盖核心逻辑
- 在 UI 中提供引擎选择选项

**Non-Goals:**
- 不支持音频格式转换（直接使用 API 返回的 WAV 格式）
- 不实现音频缓存（复用现有的 TTSCacheManager）
- 不修改现有的 MiniMax/MiMo/SAPI Provider 逻辑
- **不做可配置 voice 字段**（voice 硬编码为"曼波"）
- 不修改配置系统（ConfigManager、DataBridge、C# Config 等）
- 降级后不修改配置文件中的 ttsEngine 值

## Decisions
1. **HTTP GET 而非 POST**：Manbo API 使用 GET 请求（url 参数），使用现有的 `Network::MakeHttpsRequestAsync`。

2. **两次 HTTP 请求**：第一次请求 API 获取 audio_url，第二次从 audio_url 下载音频数据。

3. **Voice 硬编码**：Manbo API 的 voice 参数固定为"曼波"，不需要配置系统支持，简化实现。

4. **Auto 模式优先级**：auto 模式下按 manbo -> minimax -> mimo -> sapi 的优先级选择。Manbo 无需 API key，作为默认首选。

5. **Provider 降级策略**：当请求失败达到最大重试次数（3 次）时，自动降级到下一个 Provider（manbo → minimax → mimo → sapi），重置重试次数并使用新 Provider 重新尝试。

6. **音频格式处理**：API 返回的 audio_url 指向 WAV 文件，直接下载后传递给 AudioPlayer。

7. **错误处理策略**：与现有 Provider 一致，API 调用失败时设置 `available_ = false`。

8. **UI 实时显示**：在配置窗口添加 Label 显示当前实际 Provider，通过 2 秒间隔的 DispatcherTimer 轮询更新，捕获降级后的变化。

## Risks / Trade-offs
1. **API 稳定性风险**：Manbo 是第三方 API，可能存在服务不稳定的风险。已通过 `available_` 机制和降级策略来缓解。
2. **两次 HTTP 请求的延迟**：需要先获取 audio_url 再下载音频，比直接返回音频数据的 Provider 多一次网络往返。
3. **音频 URL 过期风险**：API 返回的 audio_url 可能有有效期限制，但实时请求不会遇到此问题。
4. **降级循环风险**：如果所有 Provider 都不可用，降级链会持续尝试每个 Provider。已通过 SAPI 作为最终回退来缓解。
5. **UI 轮询开销**：2 秒间隔的 DispatcherTimer 轮询对性能影响极小，但可能不是最优雅的方案。未来可考虑事件通知机制优化。
