# Why

当前实现中，非穿透模式和穿透模式共用一个 `OPACITY` 配置值，穿透模式的透明度是按固定比例（170/255 ≈ 67%）计算得出。用户无法独立控制两种模式的透明度，需要更灵活的配置选项。

# What Changes

- 新增 `penetratingModeOpacity` 配置字段，默认值 50
- 保留现有 `opacity` 配置字段（用于非穿透模式），默认值 80
- UI 新增穿透模式透明度滑块
- 业务逻辑根据窗口锁定状态选择使用对应透明度

# Capabilities

## New Capabilities

- `SeparateOpacityControl`: 用户可以分别设置非穿透模式和穿透模式的窗口透明度

## Modified Capabilities

- `OpacitySetting`: 从单一透明度控制改为双透明度控制

# Impact

修改以下文件：
- `MonsterOrderWilds/ConfigManager.h` - ConfigData 新增字段
- `MonsterOrderWilds/ConfigFieldRegistry.cpp` - 注册新字段
- `MonsterOrderWilds/DataBridgeWrapper.h` - 新增属性
- `JonysandMHDanmuTools/DataStructures.cs` - ConfigDataSnapshot 新增字段
- `JonysandMHDanmuTools/Utils.cs` - 注册字段和属性
- `JonysandMHDanmuTools/ProxyClasses.cs` - ConfigProxy 新增属性
- `JonysandMHDanmuTools/ConfigWindow.xaml` - 新增 UI 控件
- `JonysandMHDanmuTools/ConfigWindow.xaml.cs` - 绑定新控件事件
- `JonysandMHDanmuTools/OrderedMonsterWindow.xaml.cs` - 根据模式使用对应透明度
