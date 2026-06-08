## ADDED Requirements

### Requirement: UI虚拟化支持
系统 SHALL 在列表类控件上启用虚拟化以提升大数据量时的渲染性能。

#### Scenario: 列表虚拟化启用
- **WHEN** `OrderedMonsterWindow` 的 `MainList` 包含大量订单项（100+）
- **THEN** 系统只渲染视口内可见的列表项，减少内存占用和渲染开销

#### Scenario: 虚拟化滚动
- **WHEN** 用户滚动 `MainList`
- **THEN** 系统复用已创建的列表项容器，仅更新显示内容

### Requirement: 虚拟化面板配置
系统 SHALL 使用 `VirtualizingStackPanel` 作为列表的 ItemsPanel 并配置适当的虚拟化模式。

#### Scenario: 面板模式配置
- **WHEN** `MainList` 初始化时
- **THEN** `VirtualizingStackPanel.VirtualizationMode` 设置为 `Recycling` 以复用容器

#### Scenario: 延迟滚动禁用
- **WHEN** 用户拖动滚动条
- **THEN** 列表内容实时跟随滚动条位置变化，不使用延迟滚动
