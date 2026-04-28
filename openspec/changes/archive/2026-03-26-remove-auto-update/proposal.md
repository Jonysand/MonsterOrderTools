## Why

当前项目包含自动更新 app 和自动更新怪物列表的功能，这些功能依赖于外部服务器（jonysand.mynatapp.cc）。由于服务器可能不再维护或用户希望完全控制更新过程，需要移除这些自动更新功能，改为手动更新方式。

## What Changes

- **移除 UI 更新按钮**：从配置窗口中删除"更新app"和"更新怪物列表"两个按钮
- **移除命令处理逻辑**：删除 `Update` 和 `UpdateList` 命令的处理代码
- **删除更新模块**：移除 `AppUpdater` 类及其所有实现代码
- **清理网络常量**：删除仅用于更新功能的 `JONYSAND_URL` 常量

## Capabilities

### New Capabilities

此变更为功能移除，不引入新能力。

### Modified Capabilities

此变更为功能移除，不修改现有能力的需求。

## Impact

- **受影响文件**：
  - `JonysandMHDanmuTools\ConfigWindow.xaml` - 移除更新按钮 XAML
  - `JonysandMHDanmuTools\ConfigWindow.xaml.cs` - 移除按钮事件处理
  - `MonsterOrderWilds\MonsterOrderWilds.cpp` - 移除 AppUpdater 调用和命令处理
  - `MonsterOrderWilds\Network.h` - 移除 JONYSAND_URL 常量
- **删除文件**：
  - `MonsterOrderWilds\AppUpdater.cpp`
  - `MonsterOrderWilds\AppUpdater.h`
- **行为变更**：用户无法再通过界面触发自动更新，需要手动下载和替换文件进行更新
- **依赖变更**：移除对更新服务器的网络依赖
