## Why

当前UI存在卡顿问题，特别是在点怪（点击怪物订单）操作时反应迟缓。用户体验要求UI保持流畅响应，当前实现中存在的同步阻塞和不必要的UI刷新导致交互不顺畅。

## What Changes

1. 优化 `OrderedMonsterWindow.RefreshOrder()` 的列表刷新机制，使用 `ObservableCollection` 替代手动 `Items.Clear()/Add()` 操作
2. 为 `MainList` 启用 UI 虚拟化以减少大数据量时的渲染开销
3. 优化事件触发频率，避免短时间内多次触发同一刷新操作
4. 优化 `OnClickOrder` 的异步处理，避免同步阻塞UI线程
5. 支持列表项的拖拽重排功能
6. 更换应用icon

## Capabilities

### New Capabilities
- `ui-virtualization`: UI虚拟化支持，提高大数据量列表的渲染性能
- `async-ui-operations`: UI操作的异步化，避免同步阻塞

### Modified Capabilities
- `csharp-ui-layer`: 更新"用户体验优化"需求的实现细节，明确要求异步处理和虚拟化支持

## Impact

- 修改 `OrderedMonsterWindow.xaml.cs` 的列表刷新逻辑
- 修改 `OrderedMonsterWindow.xaml` 添加虚拟化支持
- 可能影响 `PriorityQueue` 和事件通知机制
