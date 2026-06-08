## ADDED Requirements

### Requirement: 事件处理节流
系统 SHALL 在短时间内对同一事件源的重复触发进行节流，避免不必要的UI更新。

#### Scenario: 队列变更事件节流
- **WHEN** `OnQueueChanged` 事件在 100ms 内触发多次
- **THEN** 系统仅执行最后一次更新，忽略中间的重复触发

#### Scenario: 正常事件处理
- **WHEN** `OnQueueChanged` 事件在 100ms 后再次触发
- **THEN** 系统正常执行刷新操作

### Requirement: 异步列表刷新
系统 SHALL 使用 `ObservableCollection` 配合 `Dispatcher.InvokeAsync` 实现UI线程安全的异步列表更新。

#### Scenario: 列表数据绑定
- **WHEN** `RefreshOrder` 执行时
- **THEN** 使用 `ObservableCollection<MonsterOrderInfo>` 作为数据源

#### Scenario: 批量更新
- **WHEN** 需要更新列表数据时
- **THEN** 系统通过 `Dispatcher.InvokeAsync` 在UI线程批量更新 `ObservableCollection`

### Requirement: 点怪操作异步化
系统 SHALL 在用户点击完成订单后，先显示操作反馈，再异步更新列表。

#### Scenario: 点击完成订单
- **WHEN** 用户点击订单项完成订单
- **THEN** 系统立即标记订单为已完成（可选：显示短暂视觉反馈），然后异步刷新列表

#### Scenario: 刷新过程不阻塞
- **WHEN** `RefreshOrder` 正在执行时
- **THEN** 用户可以继续与UI其他部分交互，不出现卡顿
