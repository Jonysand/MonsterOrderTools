## ADDED Requirements

### Requirement: 正则表达式预编译
系统 SHALL 在初始化阶段编译所有正则表达式，弹幕处理路径上不得创建新的 Regex 实例。

#### Scenario: DanmuManager 点怪 pattern 预编译
- **WHEN** DanmuManager 构造函数执行
- **THEN** 6 个点怪 pattern（`^点怪`、`^点个`、`^点只`、`^點怪`、`^點個`、`^點隻`）被编译为 `Regex[]` 并缓存

#### Scenario: DanmuManager 插队 pattern 预编译
- **WHEN** DanmuManager 构造函数执行
- **THEN** 4 个插队 pattern（`优先`、`插队`、`優先`、`插隊`）被编译为 `Regex[]` 并缓存

#### Scenario: DanmuManager 历战前缀预编译
- **WHEN** DanmuManager 构造函数执行
- **THEN** 3 个历战前缀 pattern（`^历战王`、`^历战`、`^王`）被编译为 `Regex` 并缓存

#### Scenario: MonsterData 怪物昵称 pattern 预编译
- **WHEN** `MonsterData.LoadJsonData()` 执行
- **THEN** 所有怪物昵称的正则 pattern（带 `\b` 边界）被编译为 `List<KeyValuePair<Regex, string>>`，后续匹配不再创建新 Regex 实例

#### Scenario: 弹幕处理不触发 Regex 编译
- **WHEN** 收到一条弹幕并执行完整处理流程（点怪匹配 + 插队检测 + 历战识别 + 怪物名匹配）
- **THEN** 零次 `new Regex()` 调用，所有匹配操作使用预编译的 Regex 实例
