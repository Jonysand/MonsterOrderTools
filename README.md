# 学习中

如果，您正巧参考了这份代码，还是希望能credit一下~

## 更新日志

所有变更详见 [openspec/changes/archive/](openspec/changes/archive/)

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
