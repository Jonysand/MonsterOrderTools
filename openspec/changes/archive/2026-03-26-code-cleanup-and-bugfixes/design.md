## Context

弹幕姬插件目前的弹幕处理路径存在严重性能问题：每条弹幕触发 140+ 次 Regex 编译（6个点怪 pattern + 4个插队 pattern + 130+个怪物昵称匹配）。同时代码库积累了旧插件框架残留和多处潜在 bug。此次改动是一次纯重构，不引入新功能，目标是修复 bug、提升性能、清理代码。

技术栈：C# WPF (.NET Framework)，Newtonsoft.Json，Bilibili Open Platform API。

## Goals / Non-Goals

**Goals:**
- 消除弹幕处理路径上的 Regex 重复编译开销
- 修复 XOR 解锁逻辑 bug 和其他潜在崩溃点
- 清理无法编译的死代码文件和废弃逻辑
- PriorityQueue 查找从 O(n) 优化到 O(1)
- 减少不必要的磁盘 IO

**Non-Goals:**
- 不新增任何功能
- 不改变弹幕处理的业务逻辑（匹配规则、优先级排序等保持不变）
- 不重构 UI 层（OrderedMonsterWindow 的全量刷新机制暂不改为 ObservableCollection）
- 不替换序列化库（保持 Newtonsoft.Json）
- 不处理 ToolsMainIndependent 相关的旧插件框架兼容性（直接删除）

## Decisions

### Decision 1: Regex 预编译为 static readonly 字段

**选择**: 将 `order_monster_patterns` 和 `priority_patterns_withoutOrder` 改为 `static readonly Regex[]`，在构造函数中用 `new Regex(pattern, RegexOptions.Compiled)` 初始化。`MonsterData` 中的昵称匹配同理。

**理由**: `RegexOptions.Compiled` 会将正则编译为 IL 代码，首次创建代价略高但后续匹配速度提升 10-50 倍。对热点路径来说收益巨大。

**替代方案考虑**:
- `Regex.CacheSize`：仅缓存最近使用的正则，不保证性能
- 手写字符串匹配：失去正则灵活性，改造成本高
- 源生成器（.NET 7+）：项目目标框架可能不支持

### Decision 2: PriorityQueue 增加 HashSet<string> 维护 userId

**选择**: 在 `PriorityQueue` 中增加 `HashSet<string> _userIds`，在 `Enqueue`、`Dequeue`、`Clear`、`LoadList` 时同步维护。`Contains` 方法改为查 HashSet。

**理由**: `IsRepeatUser` 在每条弹幕都调用一次，当前是 O(n) 线性遍历。HashSet 实现 O(1) 查找，且改动量很小（只需在 4 个方法中加一行同步代码）。

**风险**: `LoadList` 反序列化后需要重建 HashSet，需确保 null 检查。

### Decision 3: SaveList 节流用 DispatcherTimer 实现

**选择**: `Enqueue` 时标记 `_dirty = true`，用一个 500ms 的 timer 检查并在脏时写磁盘。同时在 `Dequeue`、`Clear` 等关键操作后立即写。

**理由**: 高密度弹幕时（比如 100人/秒），每条都写文件会阻塞处理线程。500ms debounce 足够短，不会丢失太多数据（最坏情况丢 500ms 内的入队记录），同时大幅减少 IO。

**替代方案考虑**:
- 定时保存（每 2 秒）：太慢，可能丢失较多数据
- 异步写：需要处理文件锁，复杂度高
- 不节流：当前方案，有性能问题

### Decision 4: 删除 ToolsMainIndependent.cs

**选择**: 整个文件删除。

**理由**: 该文件依赖 `BilibiliDM_PluginFramework` 基类，但类声明已被注释掉（第13行），引用了 `DanmuManager` 中不存在的方法（`OnReceivedDanmaku`、`OnReceivedRoomCount`），且有完全无用的 `using System.Security.Cryptography.X509Certificates`。它已经是死代码，保留只会造成混淆。

### Decision 5: 拼写修正作为 rename

**选择**: `OnReceicedRawJson` → `OnReceivedRawJson`，同步修改 `ToolsMain.cs:223` 的调用点。

**理由**: 这是一个单字母拼写错误，调用点只有一处，改动风险极低。

## Risks / Trade-offs

- **[Regex Compiled 内存增加]** → 每个 Compiled Regex 约占用 10-20KB 内存，总计约 3MB。对比性能收益可接受。
- **[SaveList 节流可能丢数据]** → 极端情况下应用崩溃会丢失最近 500ms 的入队记录。但队列数据本来就是直播过程中的临时数据，重启后会重新开始，可接受。
- **[XOR 修复可能影响现有行为]** → 当前 bug 可能导致某些用户发现"解锁后还是要点两次才能操作"。修复后行为会更正确，但用户可能已经适应了错误行为。需在 release notes 中说明。
- **[删除 ToolsMainIndependent.cs]** → 如果未来需要恢复旧插件框架支持，需要从 git 历史恢复。但当前该文件无法编译，保留无意义。
