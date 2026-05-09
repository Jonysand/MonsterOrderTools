# 学习中

如果，您正巧参考了这份代码，还是希望能credit一下~

## 更新日志

所有变更详见 [openspec/changes/archive/](openspec/changes/archive/)

## [v29] - 2026-05-09

### Added
- **一键黑幕（批量补签）**: 将 Python 批量补签脚本内嵌到 C++ 项目，舰长打卡 AI 配置页新增"一键黑幕"按钮
  - 功能实现于 `ProfileManager`，复用现有数据库连接和 `profilesLock_` 锁机制
  - 查询所有 `cumulative_days > 0` 的用户，自动补签从最早打卡日期到今天的连续缺失日期
  - 使用 `BEGIN IMMEDIATE` 事务包裹整个批量操作，失败全量回滚保证数据一致性
  - 补签完成后更新 `user_profiles.last_checkin_date` 和 `continuous_days`
  - 通过 `DataBridge` 导出供 C# UI 调用，结果消息通过 `MessageBox` 弹窗显示
  - 功能不在 `ONLY_ORDER_MONSTER=1`（Lite 模式）下显示

### Fixed
- **弹窗乱码修复**: 修复"一键黑幕"结果弹窗中文乱码问题
  - 根因: C++ 侧传递 UTF-8 编码字节，C# 侧 `StringBuilder` + `CharSet.Ansi` 导致系统用 GBK 解码
  - 修复: C# P/Invoke 声明移除 `CharSet.Ansi`，改用 `byte[]` 接收，`Encoding.UTF8.GetString()` 手动解码

## [v28] - 2026-05-09

### Added
- **Manbo TTS 音色选择**: 新增 184 种音色可选（除默认"曼波"外）
  - UI 新增音色选择 ComboBox，"曼波（付费）"置顶显示
  - 硬编码音色列表（184 项），覆盖新闻、动漫、方言、角色等多种风格
  - 配置字段 `manboVoice` 全栈同步（C++ ConfigData ↔ C# MainConfig）
  - 根据音色自动切换 API 端点：
    - "曼波" → `/apis/mbAIscvip`（付费音色）
    - 其他音色 → `/apis/AIvoice?speaker=<音色>`（免费音色）
  - 两种端点使用相同的 `Authorization: Bearer <key>` 鉴权和响应格式
  - 配置持久化到 JSON 和注册表，支持跨 session 保留选择

## [v27] - 2026-05-09

### Changed
- **Manbo TTS API 迁移**: 替换 Manbo TTS API 端点到 `api.milorapart.top`
  - 新 API 路径: `/apis/mbAIscvip?text=<message>&format=mp3&speed=<speed>&key=<api_key>`
  - 添加 `Authorization: Bearer <key>` 请求头鉴权
  - 响应字段从 `data.audio_url` 改为顶层 `url`，错误字段从 `message` 改为 `msg`
  - 语速映射: `speechRate`（-10~10）线性映射为 API `speed`（-50~50）

### Added
- **Manbo API Key 配置**: 新增 Manbo API Key 注册表存储和 UI 输入框
  - 注册表路径: `HKCU\Software\MonsterOrderWilds\ManboApiKey`（与 idCode 同级）
  - 全栈字段同步: C++ ConfigData → DataBridgeWrapper → C# DataStructures/Utils/ProxyClasses
  - ConfigWindow 新增 Manbo API Key 输入框（PasswordBox），位于 TTS 引擎选择框下方
  - 语音速率滑块移至 TTS 引擎设置区域

## [v26] - 2026-05-09

### Fixed
- **SAPI TTS 架构优化**: 移除回调机制，改用 `GetStatus()` 轮询检测播放状态
  - 解决回调中 Voice 对象生命周期管理的 UAF（Use-After-Free）风险
  - 添加播放超时保护，防止卡死
- **TTS 队列关键 bug 修复**: 修复弹幕密集时 TTS 请求被饿死导致 `active` 计数异常增长的问题
  - 修复 HTTP 状态码未传递及错误消息为空的问题
- **SAPI TTS 串行播放修复**: 防止弹幕密集时音频覆盖丢失

### Changed
- **本地音频触发优化**: 去掉仅限 Manbo TTS 引擎的限制，所有本地语音在任何 TTS 引擎设置下均可直接播放

## [v25] - 2026-05-05

### Added
- **特殊语音支持**: 添加本地特殊语音播放功能，可不依赖 Manbo TTS 引擎直接播放本地语音文件
  - 新增 `LocalVoiceManager`：弹幕内容匹配本地语音资源并直接播放
  - 特殊语音播放独立于 TTS 引擎选择，即使使用 MiMo/SAPI 引擎也能触发
  - 安装包包含 `local_voices.zip` 资源文件

### Fixed
- **TTS 引擎标签实时更新**: 修复 `CurrentTTSEngineLabel` 无法实时显示当前 TTS 引擎的问题
  - 根因: C++/CLI 编译导致 `__declspec(dllexport)` 产生 C++ name mangling，C# P/Invoke 找不到入口点 (`EntryPointNotFoundException`)
  - 修复: 为 `TTSManager_GetCurrentProviderName`、`DataBridge_SetAIReplyCallback`、`DataBridge_SetCheckinTTSPlayCallback` 添加 `extern "C"` 禁用 name mangling
  - 修复: `Config_SetValue` 字符串转换时保留末尾 `\0` 导致引擎名称比较失败（如 `"mimo\0" != "mimo"`），改用 `resize()` 去除终止符
  - 增强: `GlobalEventListener.Invoke` 添加 try-catch 隔离，防止单个监听器异常中断整个事件链
- **TTS 重复播报修复**: 修复 TTS 降级或切换引擎后同一条弹幕被重复播报的问题
- **TTS 手动选择失败恢复**: 修复手动选择 TTS 引擎失败后无法自动恢复原引擎的问题
- **怪物列表路径修复**: 修复 `monster_list.json` 使用工作目录相对路径导致启动时加载失败的问题，改为使用 exe 目录相对路径
- **历史订单列表加载修复**: 修复历史订单列表在启动时不加载的问题
- **小米 TTS 模型更新**: 更新小米 MiMo TTS 模型版本和怪物图标路径配置
- **点怪列表文字自动滚动修复**: 修复点怪列表中用户名和怪物名的文字自动滚动功能失效的问题

### Removed
- **移除 MiniMax 支持**: 彻底移除 MiniMax TTS Provider 和 MiniMax AI Chat Provider
  - 删除 `MiniMaxTTSProvider.cpp/h`、`MiniMaxAIChatProvider.cpp/h`
  - TTS 降级链从 `manbo → minimax → mimo → sapi` 简化为 `manbo → mimo → sapi`
  - AI Chat 默认 provider 从 `minimax` 改为 `deepseek`
  - 移除 MiniMax 相关配置字段：`minimaxVoiceId`、`minimaxSpeed`、`MINIMAX_API_KEY`
  - 移除 MiniMax UI 控件：引擎选择、音色选择（58种）、语速调节
  - 清理 C++ 和 C# 两层配置注册表、DataBridge 代理、ConfigWindow 事件处理
  - 更新 openspec 文档，移除所有 MiniMax 相关规格说明

## [v24] - 2026-04-28

### Added
- **补签卡系统**: 新增补签卡机制，舰长可通过点赞获取补签卡，用于补签缺失的打卡日期
  - 连续点赞 7 天获得 1 张补签卡（每 7 天循环发放）
  - 单日点赞突破 1000 获得 1 张补签卡（每月首次）
  - 弹幕发送"补签"命令消耗补签卡补签最近的缺失日期
  - 弹幕发送"补签查询"查看当前补签卡数量和进度
- **ONLY_ORDER_MONSTER 编译宏**: 新增仅点怪精简模式（Lite），通过 `ONLY_ORDER_MONSTER=1` 条件编译禁用非核心功能（TTS、打卡、补签卡等）
- **Manbo TTS 本地音频弹幕**: 新增本地音频弹幕匹配播放功能
  - 新增 `LocalVoiceManager`：弹幕匹配 + zip 音频读取
  - 使用 miniz 库动态解压 `local_voices.zip`
  - 匹配特定弹幕内容，从 zip 中读取预置音频播放
  - 仅在 manbo 引擎且非 ONLY_ORDER_MONSTER 模式下生效
  - 安装包在非 lite 模式下包含 `local_voices.zip`
- **WriteQueue 持久化**: 新增 WriteQueue 组件，保障数据可靠写入
- **气泡显示时间延长**: 打卡气泡显示时间从 8 秒延长至 15 秒
- **补签命令过滤**: 补签命令从 TTS 播报中过滤，避免误播
- **Voice 开关检查**: HandleSpeekDm 中添加 enableVoice 检查

### Changed
- **Git 管理优化**: `CustomizedVoice` 目录、`monster_icons` 目录独立于版本库管理，安装包通过外部资源分发
- **连接重试机制优化**: 除用户主动断开外，其余情况收到断连消息立刻反复尝试重连
  - 重连延迟从 1000ms 缩短为 0ms（立即重连）
  - 移除了重试次数上限，改为无限重连
  - 仅当用户主动断开（点击"断开"按钮）时停止重连
- **舰长打卡体验优化**: 延迟保存打卡记录至 AI 回复和 TTS 成功后，提升打卡体验
- **WebSocket 日志级别**: WebSocket cmd 日志改为 LOG_INFO，Release 模式可见
- **日志按日期分文件**: 日志按日期分文件存储到 Logs 目录
- **累计天数计算重构**: 统一累计天数计算逻辑到 ProfileManager::CalculateCumulativeDays
- **怪物列表更新**: 更新怪物列表和安装包配置

### Fixed
- **Git 配置修复**: gitignore 规则全面梳理，隐藏文件/目录不再被意外跟踪
- **语音播报修复**: 修复语音播报失效及打卡功能异常
- **补签卡机制修复**: 修复 code review P0/P1 问题 - 补签卡机制关键 bug 修复
- **TTS 崩溃修复**: 使用 make_shared 替代 make_unique 修复 SEHException 崩溃
- **TTS 异步安全恢复**: 恢复 TTS 异步安全机制和 LoadStopWords 停用词加载
- **点怪列表图标恢复**: 恢复点怪列表图标显示功能
- **补签查询优化**: 简化补签查询文本，分行显示关键信息；结果仅显示气泡不触发 TTS 朗读
- **补签重复气泡防止**: 防止补签重复气泡显示
- **TTS 失败降级**: 补签场景添加 TTS 失败降级处理
- **累计天数覆盖修复**: 修复 `CaptainCheckInModule` 保存 profile 时漏掉 `cumulativeDays` 字段导致累计天数被覆盖为 0 的问题，新增数据补偿机制自动从 `checkin_records` 表恢复
- **Code Review 修复**: 指数退避、日志脱敏、异步异常处理

## [v23] - 2026-04-25

### Added
- **独立卸载程序**: 新增独立卸载程序，支持清理注册表数据和残留文件
- **点怪列表自动滚动**: 点怪列表用户名和怪物名添加自动滚动效果，解决长名显示不全问题

### Changed
- **MiMo Style 标签优化**: 移除预设 style 标签，改为弹幕内 `(风格)` 标签动态实时提取，更灵活适配不同语气需求
- **DeepSeek 模型更新**: AI 对话模型更新为 `deepseek-v4-flash`，提升回复质量和响应速度

### Fixed
- **TTS 引擎切换崩溃**: 修复多次切换 TTS 引擎（Manbo/MiniMax/MiMo/SAPI）导致的闪退崩溃问题

## [v22] - 2026-04-25

### Added
- **怪物图标本地化**: 将173个怪物的图标从网络URL迁移到本地zip资源包
  - 新增 `MonsterIconLoader` 组件：从 `monster_icons.zip` 流读取图标，支持内存缓存
  - 建立75个em编号到zip路径的完整映射表
  - 补全59个原无URL怪物的图标路径
  - 修正11个怪物的错误图标映射（大痹贼龙→Great_Girros、凶爪龙→Ebony_Odogaron等）
  - 新增12个历战/历战王怪物的特殊图标（Tempered/Arch-Tempered）
- **安装包集成图标资源**: 修改 `installer/build.py` 和 `MonsterOrderWilds.iss`，安装时自动复制 `monster_icons.zip`
- **打卡记录导出**: 新增导出打卡记录功能，支持按打卡天数降序排列

### Changed
- **图标加载方式**: `OrderedMonsterWindow` 图标从 `Uri` 网络加载改为 `BitmapImage` zip流加载
- **图标路径格式**: `monster_list.json` 中所有图标地址改为zip内相对路径（如 `MHWilds/MHWilds-Rathalos_Icon.png`）

### Fixed
- **C++日志格式**: 修复 `MonsterOrderWilds.cpp` 中 `%s` 期望宽字符导致UTF-8日志乱码的问题（改为 `%hs`）
- **Zip流读取**: 修复 `ZipArchiveEntry` 流不可seek导致 `BitmapImage.Freeze()` 失败的问题（先复制到 `MemoryStream`）

## [v21] - 2026-04-24

### Fixed
- **TTS引擎选择修复**: 修复 `std::lock_guard` 递归锁定崩溃、`XiaomiTTSProvider::HashtagToStyle` 迭代器失效、TTS引擎选择UI配置无法持久化
- **代码审查修复**: 修复多线程安全问题（StringProcessor缓存、CredentialsManager全局变量、TextToSpeech队列数据竞争、ConfigManager锁问题），重构ToolsMain配置处理为字典分发，PriorityQueueManager原子写入
- **停用词词典加载**: 修复 jieba 词典加载路径问题

### Added
- **累计打卡天数**: 新增累计打卡天数存储、读取、AI提示词包含，数据库向后兼容

## [v20] - 2026-04-19

### Added
- **Manbo TTS Provider**: 新增 Manbo TTS 引擎集成，支持自动降级（Manbo → MiniMax → MiMo → SAPI），UI显示当前实际引擎
- **打卡记录导出**: 新增导出打卡记录功能，支持按打卡天数降序排列

### Fixed
- **Manbo TTS 修复**: 修复 TTSProviderFactory 缺少 manbo/minimax 处理、Manbo API URL 查询参数未正确处理、TTSResponse 缺少 format 字段

## [v19] - 2026-04-16

### Added
- **运行时TTS引擎切换**: 支持程序运行时动态切换TTS引擎，无需重启

### Changed
- **移除 USE_MIMO_TTS 宏**: 简化代码，消除条件编译分支，TTS功能始终启用

## [v18] - 2026-04-15

### Added
- **MiniMax TTS UI**: 新增 MiniMax 引擎选择、音色选择（58种）、语速调节
- **TTS凭证拆分**: credentials.dat 支持 mimo_tts_api_key 和 minimax_tts_api_key 独立字段

## [v17] - 2026-04-14

### Added
- **舰长签到异步处理**: AI回复异步生成，锁粒度优化，数据库操作异步化，高并发场景UI保持响应
- **TTS Provider解耦**: TTSManager 通过 ITTSProvider 接口调用，移除 MimoTTSClient 直接依赖

## [v16] - 2026-04-13

### Added
- **独立透明度控制**: 分别设置非穿透模式和穿透模式的窗口透明度

## [v15] - 2026-04-12

### Fixed
- **去重打卡数据库写入**: 消除首次打卡时重复的Profile保存，将profile保存和打卡记录写入合并为单一原子操作，同时修复内存/DB不一致问题

## [v14] - 2026-04-08

### Added
- **AI回复气泡**: 舰长打卡AI回复在点怪窗口显示气泡，支持渐显渐隐动画、多气泡堆叠
- **异步网络请求**: 网络请求从协程改为 callback 模式

### Fixed
- **TTS连接修复**: 修复 std::mutex 崩溃（shared_ptr/weak_ptr）、移除 WINHTTP_FLAG_ASYNC、SAPI默认音量改为50、MiMo API Key字段统一、GetAI_PROVIDER() JSON构建

## [v13] - 2026-04-06

### Added
- **舰长打卡AI回复系统**: 学习舰长发言习惯，检测打卡触发词，调用MiniMax文本对话API生成个性化回复，支持TTS播报

## [v12] - 2026-04-04

### Added
- **凭证加密存储**: credentials.dat 明文凭证改为加密存储，Base64解码 + zlib解压
- **TTS缓存管理**: 按日期分目录存储缓存，启动时自动清理过期缓存
- **礼物连击优化**: 全局冷却时间 + 动态连击检测，防止刷屏

## [v11] - 2026-03-31

### Added
- **UI流畅度优化**: 异步UI操作、列表虚拟化渲染、事件节流

## [v10] - 2026-03-30

### Added
- **C++数据处理层**: 统一配置管理、怪物数据处理、优先级队列管理、弹幕业务逻辑处理、事件通知机制

## [v9] - 2026-03-28

### Added
- **MiMo V2 TTS**: 替换原有TTS为小米MiMo-V2-TTS API
- **方言支持**: 支持选择不同方言音色
- **角色语音**: 支持选择不同角色语音风格

### Changed
- **连接状态机**: 优化连接状态管理和重连逻辑

## [v8] - 2026-03-26

### Added
- **锁定窗口按钮与热键同步**: 事件机制确保状态同步
- **窗口解锁位运算修复**: 使用AND-NOT操作清除WS_EX_TRANSPARENT标志位

### Fixed
- **反序列化null安全**: 队列文件损坏或为空时初始化为空列表
- **ConfigChanged参数校验**: 跳过无冒号分隔的消息分支
- **拼写修正**: GetInst() 拼写修正
- **移除废弃代码**: ToolsMainIndependent旧插件入口、medalName废弃字段、ORDER_FINISH_CLICK_INTERVAL未使用常量

### Removed
- **移除自动更新**: 移除自动更新功能及相关代码
