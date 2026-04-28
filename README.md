# 学习中

如果，您正巧参考了这份代码，还是希望能credit一下~

## 更新日志

所有变更详见 [openspec/changes/archive/](openspec/changes/archive/)

## [Unreleased]

## [v22] - 2026-04-25

### Added
- **怪物图标本地化**: 将173个怪物的图标从网络URL迁移到本地zip资源包
  - 新增 `MonsterIconLoader` 组件：从 `monster_icons.zip` 流读取图标，支持内存缓存
  - 建立75个em编号到zip路径的完整映射表
  - 补全59个原无URL怪物的图标路径
  - 修正11个怪物的错误图标映射（大痹贼龙→Great_Girros、凶爪龙→Ebony_Odogaron等）
  - 新增12个历战/历战王怪物的特殊图标（Tempered/Arch-Tempered）
- **安装包集成图标资源**: 修改 `installer/build.py` 和 `MonsterOrderWilds.iss`，安装时自动复制 `monster_icons.zip`

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
