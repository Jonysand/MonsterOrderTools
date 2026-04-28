## 0. 单元测试

> **实施优先级**: 优先编写，确保原有功能正常实施

- [x] 0.1 创建 OrderedMonsterWindowTests.cs 测试文件
- [x] 0.2 编写 ObservableCollection.Move() 功能测试
- [x] 0.3 编写节流逻辑时间验证测试
- [x] 0.4 编写 Dispatcher.InvokeAsync 异常处理测试
- [x] 0.5 运行单元测试验证所有测试通过

**测试文件**: `JonysandMHDanmuTools.Tests\OrderedMonsterWindowTests.cs`

**测试内容**:
- `TestObservableCollectionMove_ValidIndices_MovesItem`: 验证 ObservableCollection.Move() 在有效索引下正确移动元素
- `TestObservableCollectionMove_SameIndex_NoChange`: 验证相同索引 Move 不产生变化
- `TestObservableCollectionMove_AdjacentIndices_MovesCorrectly`: 验证相邻索引 Move 正确移动
- `TestThrottleLogic_Within100Ms_IgnoresSecondCall`: 验证100ms内的重复调用被忽略
- `TestThrottleLogic_After100Ms_Executes`: 验证超过100ms后正常执行
- `TestThrottleLogic_MultipleRapidCalls_OnlyFirstExecutes`: 验证多次快速调用仅第一次执行
- `TestDispatcherInvokeAsync_CallsAction`: 验证 Dispatcher.InvokeAsync 正确调用 Action
- `TestDispatcherInvokeAsync_ReturnsTask`: 验证返回 Task 类型
- `TestObservableCollection_NotifyCollectionChanged_OnMove`: 验证 Move 触发 NotifyCollectionChanged
- `TestObservableCollection_Clear_RemovesAllItems`: 验证 Clear 移除所有元素
- `TestObservableCollection_Add_IncreasesCount`: 验证 Add 增加计数
- `TestObservableCollection_Remove_DecreasesCount`: 验证 Remove 减少计数

**测试结果**: 12/12 通过

## 1. UI虚拟化配置

- [x] 1.1 修改 OrderedMonsterWindow.xaml，为 MainList 添加 VirtualizingStackPanel 作为 ItemsPanel
- [x] 1.2 配置 VirtualizationMode="Recycling" 和 IsVirtualizing="True"

**实现方案**:
- 在 `ListView` 内添加 `ItemsPanel` 属性，使用 `VirtualizingStackPanel`
- 配置 `VirtualizationMode="Recycling"` 和 `IsVirtualizing="True"`

## 2. ObservableCollection 集成

- [x] 2.1 在 OrderedMonsterWindow.xaml.cs 中添加 ObservableCollection<MonsterOrderInfo> 字段
- [x] 2.2 修改 RefreshOrder() 方法，使用 ObservableCollection 替代 Items.Clear/Add 模式
- [x] 2.3 将 MainList.ItemsSource 绑定到 ObservableCollection 实例
- [x] 2.4 修改 MainList_Drop 使用 ObservableCollection.Move() 替代 Items.Remove/Insert

**实现方案**:
- 字段: `private ObservableCollection<MonsterOrderInfo> _orderCollection = new ObservableCollection<MonsterOrderInfo>();`
- ItemsSource绑定: 在代码behind中设置 `MainList.ItemsSource = _orderCollection`（方案B）
- RefreshOrder(): 使用 `Dispatcher.InvokeAsync` 批量更新 `_orderCollection`
- MainList_Drop: 使用 `_orderCollection.Move(oldIndex, newIndex)` 替代 Remove/Insert

## 3. 事件节流机制

- [x] 3.1 在 OrderedMonsterWindow.xaml.cs 中添加节流时间戳和常量
- [x] 3.2 修改 OnQueueChanged 事件处理，添加 100ms 节流逻辑

**实现方案**:
- 常量: `private const int THROTTLE_MS = 100;`
- 字段: `private DateTime _lastRefreshTime = DateTime.MinValue;`
- 节流逻辑: 比较当前时间与 `_lastRefreshTime`，间隔小于100ms时忽略

## 4. 异步操作优化

- [x] 4.1 修改 OnClickOrder 方法，实现异步刷新
- [x] 4.2 确保 Dispatcher.InvokeAsync 正确处理异常

**实现方案**:
- OnClickOrder 改为 `async` 方法
- 使用 `await Dispatcher.InvokeAsync(() => RefreshOrder());`
- 异常处理: 保持 RefreshOrder 内部现有 try-catch 机制

## 5. 验证测试

- [x] 5.1 运行单元测试确保功能无误
- [x] 5.2 编译项目验证无错误（Debug 配置）
- [ ] 5.3 运行测试验证 UI 响应性提升（手动验证）
- [ ] 5.4 测试拖拽重排功能在虚拟化启用后正常工作（手动验证）

**单元测试命令**: `dotnet test JonysandMHDanmuTools.Tests --configuration Debug`
**编译命令**: `MSBuild.exe "JonysandMHDanmuTools.sln" -p:Configuration=Debug -p:Platform=x64 -t:Build -m`

**配置说明**:
- Debug|x64: SUBSYSTEM=Windows, 无 RUN_UNIT_TESTS → 运行主程序
- Release|x64: SUBSYSTEM=Windows, 无 RUN_UNIT_TESTS → 运行主程序
- UnitTest|x64: SUBSYSTEM=Console, 有 RUN_UNIT_TESTS → 运行单元测试

## 6. 跑马灯动画优化

- [x] 6.1 修改 XAML，移除 EventTrigger 自动启动的 Storyboard
- [x] 6.2 添加 Storyboard.Completed 事件绑定
- [x] 6.3 在 C# 代码中添加动画控制字段和状态管理
- [x] 6.4 实现 StartMarqueeAnimation(bool loop) 方法
- [x] 6.5 实现 OnMarqueeAnimationCompleted 回调处理队列
- [x] 6.6 修改 AddRollingInfo 方法，根据当前状态处理新信息
- [x] 6.7 删除不再需要的 DispatcherTimer
- [x] 6.8 验证跑马灯功能正常工作

**实现方案**:

**状态定义**:
- `_isShowingDefault: bool` - 当前是否显示默认信息
- `_currentAnimationIsLoop: bool` - 当前动画是否循环模式

**动画控制逻辑**:

| 当前状态 | 新信息到达 | 处理方式 |
|----------|-----------|----------|
| 显示默认信息 | 是 | 立即切换到新信息，动画改为1x |
| 显示队列信息 | 是 | 加入队列等待 |
| 动画完成 | 队列有内容 | 弹出队首，显示并重启动画（1x） |
| 动画完成 | 队列空 | 恢复默认信息，动画改为循环 |

**关键方法**:
- `StartMarqueeAnimation(bool loop)`: 停止当前动画，设置新的 From/To 值，重启动画
- `OnMarqueeAnimationCompleted(object sender, EventArgs e)`: 动画完成回调，处理队列逻辑
- `AddRollingInfo(RollingInfo rollingInfo)`: 添加信息到队列，根据当前状态决定立即显示或等待

**动画参数**:
- 默认信息: `RepeatBehavior="Forever"` (循环滚动)
- 队列信息: `RepeatBehavior="1x"` (滚动一次后切换)

## 决策记录

| 决策点 | 选择 | 理由 |
|--------|------|------|
| ItemsSource绑定方式 | B (代码behind) | 更简单，符合现有代码风格 |
| 节流实现方式 | B (时间戳比较) | 精确控制100ms间隔 |
| OnClickOrder异常处理 | 保持现状 | RefreshOrder已有try-catch |
| Newtonsoft.Json | 已移除 | JsonPropertyAttribute 等为死代码，从未被序列化调用；packages目录已清理 |
| 跑马灯动画驱动 | Storyboard.Completed | 动画完成时自动触发，无需Timer同步 |
| 新信息+显示默认 | 立即切换 | 用户期望即时反馈 |
| 队列信息切换 | 直接切换 | 文字滚动到头后直接显示下一条，无需滚回 |

## 7. 跑马灯默认文本配置

- [x] 7.1 C++ ConfigManager.h: ConfigData 添加 defaultMarqueeText 字段
- [x] 7.2 C++ ConfigFieldRegistry.cpp: 注册 defaultMarqueeText 字段
- [x] 7.3 C++ ConfigManager.cpp: JSON 加载/保存处理
- [x] 7.4 C++/CLI DataBridgeWrapper.h: ConfigProxy 添加属性
- [x] 7.5 C# DataStructures.cs: ConfigDataSnapshot 添加字段
- [x] 7.6 C# Utils.cs: MainConfig 添加 DEFAULT_MARQUEE_TEXT 属性
- [x] 7.7 C# ProxyClasses.cs: ConfigProxy 添加属性
- [x] 7.8 C# ToolsMain.cs: ConfigChanged 处理 DEFAULT_MARQUEE_TEXT
- [x] 7.9 C# ConfigWindow.xaml: 添加 TextBox（其他设置最开头）
- [x] 7.10 C# ConfigWindow.xaml.cs: TextChanged 事件处理
- [x] 7.11 C# OrderedMonsterWindow.xaml.cs: 使用配置值替代硬编码常量

**实现方案**:

**UI 位置**: "其他设置" 标签页最开头

**默认值**: `发送'点怪 xxx'进行点怪`

**配置键名**: `DEFAULT_MARQUEE_TEXT` (C#) / `defaultMarqueeText` (C++)

## 8. 功能验证

- [x] 8.1 跑马灯动画优化验证
  - [x] 8.1.1 发送弹幕时，新信息立即显示并滚动
  - [x] 8.1.2 滚动完成后自动切换下一条信息
  - [x] 8.1.3 所有队列消息滚动完成后恢复默认信息循环

- [x] 8.2 跑马灯默认文本配置验证
  - [x] 8.2.1 设置界面显示默认文本输入框
  - [x] 8.2.2 修改默认文本后，跑马灯立即更新
  - [x] 8.2.3 重启后默认文本配置保持

- [x] 8.3 配置文件持久化验证
  - [x] 8.3.1 修改任意配置后关闭程序，重新打开配置保持
  - [x] 8.3.2 修改 ID_CODE 后连接按钮功能正常
  - [x] 8.3.3 TTS 语音开关状态保存正常

**重要**: `MonsterOrderWilds_configs` 目录必须与 `MonsterOrderWilds.exe` 在同一目录下，否则配置文件无法正确读写。