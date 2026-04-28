## Context

当前 `OrderedMonsterWindow` 的 `RefreshOrder()` 方法使用 `MainList.Items.Clear()` 配合循环 `MainList.Items.Add()` 更新列表。这种方式会导致每次 Add 都触发 UI 重新渲染，在列表较大时造成明显卡顿。

`OnClickOrder` 事件处理直接同步调用 `RefreshOrder()`，在UI线程上执行排序和列表刷新操作，导致点击响应延迟。

事件通知机制 (`EventDispatcher.Instance.OnQueueChanged`) 在队列变化时无节制地触发刷新，可能导致短时间内多次重绘。

## Goals / Non-Goals

**Goals:**
- 消除点怪操作后的UI卡顿，实现60fps的流畅响应
- 减少不必要的UI刷新和线程切换
- 优化大数据量（100+订单）时的列表滚动性能
- 保持拖拽重排功能的正常工作

**Non-Goals:**
- 不重构优先级队列的排序算法（排序本身不是瓶颈）
- 不改变现有的用户交互逻辑和视觉外观
- 不引入新的外部依赖库
- 不修改点怪的核心业务逻辑

## Decisions

### Decision 1: 使用 ObservableCollection 替代手动 Items 操作

**选择:** 使用 `ObservableCollection<MonsterOrderInfo>` 作为 `MainList.ItemsSource`

**理由:**
- `ObservableCollection` 实现 `INotifyCollectionChanged`，自动通知UI更新
- 配合 `Dispatcher.InvokeAsync` 可批量合并UI更新
- 比手动 Clear/Add 减少 N 次绑定更新为 1 次批量更新
- 与虚拟化面板兼容，可共同使用提升性能

**替代方案:**
- `BindingOperations.EnableCollectionSynchronization` - 复杂，不适合此场景
- 直接操作 Items 配合 Freeze - 仍会触发多次渲染

**影响:**
- 拖拽重排 `MainList_Drop` 需要从直接操作 `MainList.Items` 改为操作 `ObservableCollection`
- 拖拽时获取索引位置的方式需从 `MainList.Items.IndexOf()` 改为 `ObservableCollection.IndexOf()`

### Decision 2: 为 MainList 启用虚拟化

**选择:** 使用 `VirtualizingStackPanel` 作为 ItemsPanel

**理由:**
- WPF 默认列表渲染所有可见项，虚拟化后只渲染视口内项目
- 列表项包含图标和文本，渲染成本高，虚拟化效果明显
- 配置简单，只需修改 XAML

**替代方案:**
- `ScrollViewer.IsDeferredScrollingEnabled` - 改善滚动体验但不减少渲染
- 自定义虚拟面板 - 过度工程

### Decision 3: 事件节流（Debounce）机制

**选择:** 在事件处理中添加简单的节流标志

**理由:**
- 当前事件可能在同一操作中触发多次（如一次弹幕同时触发队列变更和刷新）
- 简单标志位即可解决问题，无需引入 Timer 或第三方库
- 不会增加显著复杂性

**替代方案:**
- `Rx` 的 `Throttle` - 引入新依赖
- `DispatcherTimer` 延迟 - 增加复杂度

### Decision 4: OnClickOrder 异步化

**选择:** `OnClickOrder` 调用 `Dequeue` 后使用 `await` 等待 `RefreshOrder` 完成

**理由:**
- `Dequeue` 操作本身快速，但后续 `RefreshOrder` 会重新渲染
- 异步化后可先显示完成反馈，再异步更新列表
- 配合 `Dispatcher.InvokeAsync` 确保UI线程安全

**替代方案:**
- 单独线程处理 - 需要手动 `Dispatcher.Invoke`，更复杂
- 不改变 - 会继续阻塞

### Decision 5: 拖拽重排支持 ObservableCollection

**选择:** 修改 `MainList_Drop` 和相关方法，操作 `ObservableCollection` 而非 `MainList.Items`

**理由:**
- `ObservableCollection` 支持 `Move()` 方法，可安全地将项目从一个索引移动到另一个索引
- 虚拟化列表中拖拽需要获取正确的容器和索引位置
- 保持与排序逻辑的一致性，避免数据源与显示不同步

**实现要点:**
- `MainList_PreviewMouseDown`: 获取 `ListViewItem` 后通过 `ItemContainerGenerator` 从 `ObservableCollection` 获取正确索引
- `MainList_Drop`: 使用 `dataItem = e.Data.GetData(typeof(MonsterOrderInfo))` 获取数据后，通过 `ObservableCollection.IndexOf()` 找到位置
- `ObservableCollection.Move()` 执行重排操作，自动触发 UI 更新

**替代方案:**
- 在拖拽时临时切换回 Items 操作 - 复杂且容易出错
- 禁用虚拟化进行拖拽 - 失去虚拟化性能优势

## Risks / Trade-offs

[Risk] `ObservableCollection` 多线程访问
→ **Mitigation**: 所有 UI 更新通过 `Dispatcher.InvokeAsync` 封装，确保在 UI 线程执行

[Risk] 虚拟化可能导致列表滚动时加载延迟
→ **Mitigation**: 使用 `VirtualizingStackPanel.IsVirtualizing="True"` 并设置合理的缓存 `VirtualizingStackPanel.VirtualizationMode="Recycling"`

[Risk] 节流可能掩盖真正的多事件问题
→ **Mitigation**: 节流仅针对同一事件源的短时间重复触发，不影响正常的事件处理流程

[Risk] 拖拽过程中虚拟化可能导致目标位置计算不准确
→ **Mitigation**: 需要在实际测试中验证 `HitTest` 结果的准确性

## Open Questions

1. `RefreshOrder` 中的 `PriorityQueue.GetInst().SortQueue()` 是否可以进一步优化？需要性能测试确认排序是否也为瓶颈。
