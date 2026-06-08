## Context

Manbo TTS 引擎目前仅支持单一音色"曼波"，通过 `/apis/mbAIscvip` 端点请求。API 提供商 (`api.milorapart.top`) 已开放约 200 个免费音色，可通过 `/apis/AIvoice/?type=list` 获取列表，并通过 `/apis/AIvoice?speaker=<name>&text=<text>` 请求特定音色。

项目采用 C++/CLI 混合架构：C++ 核心层负责网络请求和 TTS 合成，C# WPF 层负责 UI。配置通过 `ConfigManager`（C++ JSON 持久化）和 `MainConfig`（C# 内存模型）双向同步，中间通过 `DataBridgeWrapper`（C++/CLI）桥接。

## Goals / Non-Goals

**Goals:**
- 用户可以在 UI 中浏览并选择 Manbo TTS 音色
- "曼波"音色置顶并标记"付费"，其余音色按 API 返回顺序排列
- 选择的音色持久化到 JSON 配置，跨会话保持
- C++ 层根据音色自动切换 API 端点（`/apis/mbAIscvip` vs `/apis/AIvoice`）
- 旧配置无此字段时默认回退到"曼波"
- API 获取失败时 UI  gracefully 降级（仅显示曼波）

**Non-Goals:**
- 不支持在 `ONLY_ORDER_MONSTER=1`（lite 模式）下使用
- 不缓存音色列表到磁盘（每次启动重新获取）
- 不实现音色搜索/过滤功能
- 不改变现有 MiMo/SAPI TTS 引擎的行为
- 不修改音频下载和播放逻辑

## Decisions

**1. C# 端获取音色列表，C++ 端执行 TTS 请求**
- *Rationale*: 音色列表是纯 UI 数据，不涉及业务逻辑。C# 层有现成的 HTTP 客户端（`HttpClient`/`WebClient`），异步获取更自然。C++ 层只关心最终选择的音色值和对应的 API 端点。
- *Alternative considered*: C++ 端获取列表并通过 DataBridge 暴露给 C#。Rejected：增加跨层复杂度，且 C# 已有异步 HTTP 能力。

**2. 使用现有配置系统传递音色选择**
- *Rationale*: `manboVoice` 作为 `ConfigData` 的字符串字段，可通过现有 `ConfigFieldRegistry` 和 `DataBridgeWrapper` 自动同步，无需新增跨层接口。
- *Alternative considered*: 新增专门的跨层 API（如 `NativeImports.GetManboVoiceList()`）。Rejected：过度设计，配置系统已满足需求。

**3. 音色列表仅内存缓存，不持久化**
- *Rationale*: API 列表可能随时更新，持久化会导致列表陈旧。每次启动获取保证最新。
- *Trade-off*: 无网络时无法显示完整列表，但已选择的音色仍可通过配置字段保留。

**4. "曼波"硬编码为默认音色**
- *Rationale*: 兼容现有行为。旧配置无 `MANBO_VOICE` 字段时，默认"曼波"确保 TTS 继续使用 `/apis/mbAIscvip` 端点。
- *Alternative considered*: 从 API 获取第一个音色作为默认。Rejected：破坏现有行为，且"曼波"是付费标识需要保留。

**5. 配置中的音色不在列表中时保留原值**
- *Rationale*: 用户可能之前选择了一个音色，API 暂时移除后又恢复。保留原值避免用户设置丢失。
- *Alternative considered*: 强制回退到"曼波"。Rejected：用户选择可能是有意的，API 列表变化不应静默覆盖用户设置。

## Risks / Trade-offs

**[Risk] API 端点变更或下线**
→ *Mitigation*: 使用统一的 `ParseApiResponse()` 解析逻辑，若端点响应格式变化只需修改一处。添加日志记录方便排查。

**[Risk] 音色名称包含特殊字符导致 URL 编码问题**
→ *Mitigation*: 使用现有的 `UrlEncode()` 函数对 `speaker` 参数进行编码。

**[Risk] 大量音色（~200个）导致 ComboBox 性能问题**
→ *Mitigation*: 200 个条目对 WPF ComboBox 是可接受的。如未来增长到数千个，可考虑虚拟化或搜索框。

**[Risk] C# 异步获取列表时用户已打开设置窗口**
→ *Mitigation*: 在 `ToolsMain` 启动时发起请求，设置窗口打开时列表通常已加载完成。若未加载完成，显示"加载中..."或仅显示曼波。

**[Trade-off] 网络依赖**
每次启动需访问 API 获取列表。无网络时列表不完整，但不影响已保存的音色使用（C++ 层直接请求，不依赖列表）。
