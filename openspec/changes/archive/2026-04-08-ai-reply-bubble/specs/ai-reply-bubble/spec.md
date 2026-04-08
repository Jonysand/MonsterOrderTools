# AI Reply Bubble Display - Spec

## ADDED Requirements

### Requirement: AI 回复气泡显示

当舰长发送打卡消息并触发 AI 回复时，在点怪窗口上方显示气泡，显示用户名和回复内容。

#### Scenario: 气泡渐显动画
- **WHEN** AI 回复生成完成
- **THEN** 气泡从透明度 0 渐显到透明度 1，持续 500ms (ease-out)

#### Scenario: 气泡停留后渐隐
- **WHEN** 气泡显示满 5 秒
- **THEN** 气泡从透明度 1 渐隐到 0，持续 800ms，然后从 Canvas 中移除

#### Scenario: 新气泡推动旧气泡
- **WHEN** 已有气泡显示时，新的 AI 回复到达
- **THEN** 新气泡从底部加入（离窗口最近），旧气泡向上移动（Y 坐标减小新气泡高度 + 8px 间距）

#### Scenario: 气泡队列上限
- **WHEN** 气泡数量达到 5 个
- **AND** 新的 AI 回复到达
- **THEN** 最旧的气泡（最上方）被移除，新气泡加入到底部

#### Scenario: 快速连续回复
- **WHEN** 多个 AI 回复在小于 500ms 内连续到达
- **THEN** 每个回复都触发独立的气泡渐显动画，气泡垂直堆叠显示

#### Scenario: 气泡内容显示
- **WHEN** 气泡显示时
- **THEN** 显示用户名（顶部，小字，淡色）和回复内容（主体，白色文字）

#### Scenario: 窗口关闭时清理
- **WHEN** 窗口关闭
- **THEN** 所有 DispatcherTimer 停止，气泡从 Canvas 移除，资源正确释放

#### Scenario: 窗口关闭时动画处理
- **WHEN** 窗口关闭
- **AND** 气泡正在执行动画（渐显或渐隐）
- **THEN** 动画立即停止，气泡直接移除，无需等待动画完成

#### Scenario: 气泡停留计时器正常完成
- **WHEN** 气泡显示满 5 秒，计时器触发
- **THEN** 计时器停止并从 _bubbleTimers 列表移除，渐隐动画开始播放，动画完成后气泡从 Canvas 和 _bubbles 列表中移除

#### Scenario: C++ 回调异常处理
- **WHEN** C++ 层调用 AIReplyCallback 时发生异常
- **THEN** 异常被捕获并记录日志，回调调用不会崩溃或中断程序

#### Scenario: 字符串指针有效期保障
- **WHEN** C++ 层准备调用 AIReplyCallback
- **THEN** 使用局部 `std::wstring` 持有用户名和内容，确保字符串指针在回调期间始终有效，回调完成后局部变量自动销毁

#### Scenario: Canvas 空队列边界
- **WHEN** 第一个气泡加入空的 Canvas
- **THEN** 新气泡正常添加到队列，设置 Canvas.Top = 0，动画正常播放

#### Scenario: 气泡内容过长换行
- **WHEN** 气泡内容过长
- **THEN** 使用 TextWrapping 自动换行，气泡高度自适应，气泡最大宽度不超过 380px

#### Scenario: 队列满时操作顺序
- **WHEN** 气泡数量已达 5 个上限
- **AND** 新的 AI 回复到达
- **THEN** 先移除最旧的气泡（最上方），再将新气泡添加到队列底部（Y=0），最后更新所有气泡位置

#### Scenario: 新气泡加入位置
- **WHEN** 新气泡加入
- **THEN** 新气泡添加到队列末尾（视觉最上方，Canvas.Top = 0），原有气泡 Canvas.Top 值增加后视觉上向下移动（推远）

### Requirement: AI 回复回调机制

C++ 层的 AI 回复结果需要传递到 C# UI 层。

#### Scenario: AI 回复回调触发
- **WHEN** `CaptainCheckInModule::PushDanmuEvent()` 生成回复成功
- **THEN** 调用注册的 `AIReplyCallback` 回调，传递用户名和回复内容

#### Scenario: 回调线程安全
- **WHEN** C++ 层调用 AIReplyCallback
- **AND** 多个线程同时设置或调用回调
- **THEN** 使用 std::mutex 保护回调指针的读写，回调调用在锁外执行

#### Scenario: 跨线程 UI 调度
- **WHEN** C++ 回调在非 UI 线程触发
- **THEN** C# 层通过 Dispatcher.InvokeAsync 将气泡操作调度到 UI 线程执行

### Requirement: 后续评估项

#### Scenario: 关联模块线程安全评估
- **NOTE** `g_danmuProcessedCallback` 存在与 `g_aiReplyCallback` 相同的线程安全问题，建议后续使用相同模式（std::mutex + 锁外调用）进行统一保护

#### Scenario: 窗口尺寸边界
- **WHEN** 窗口尺寸较小（高度不足以显示完整气泡）
- **THEN** 气泡基于实际窗口尺寸渲染，内容可能部分不可见但不会导致窗口或应用崩溃

#### Scenario: 气泡数量内存压力
- **WHEN** 在极端情况下快速收到大量 AI 回复（远超 5 个上限处理能力）
- **THEN** 由于队列限制为 5 个，超出部分被立即丢弃，内存使用量保持在可控范围内