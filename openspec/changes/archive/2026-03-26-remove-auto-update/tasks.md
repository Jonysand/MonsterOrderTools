## 1. 删除 AppUpdater 模块

- [x] 1.1 删除 `MonsterOrderWilds\AppUpdater.cpp` 文件
- [x] 1.2 删除 `MonsterOrderWilds\AppUpdater.h` 文件

## 2. 清理 MonsterOrderWilds.cpp

- [x] 2.1 删除第 4 行的 `#include "AppUpdater.h"`
- [x] 2.2 删除第 26 行的 `AppUpdater::Inst()->InitModule();`
- [x] 2.3 删除第 68 行的 `AppUpdater::Inst()->Tick();`
- [x] 2.4 删除第 100-104 行的 Update 和 UpdateList 命令处理代码

## 3. 清理 Network.h

- [x] 3.1 删除第 16 行的 `JONYSAND_URL` 常量定义

## 4. 清理 ConfigWindow UI

- [x] 4.1 从 `ConfigWindow.xaml` 中删除第 315-338 行的两个更新按钮定义
- [x] 4.2 从 `ConfigWindow.xaml.cs` 中删除 `UpdateButton_Click` 事件处理（第 171-174 行）
- [x] 4.3 从 `ConfigWindow.xaml.cs` 中删除 `UpdateListButton_Click` 事件处理（第 176-179 行）

## 5. 验证和测试

- [x] 5.1 检查项目文件（.vcxproj）是否引用了 AppUpdater.cpp/h，如有则删除引用
- [x] 5.2 编译项目确保无语法错误
- [ ] 5.3 运行程序确认配置窗口不再显示更新按钮
- [ ] 5.4 验证核心功能（弹幕处理、怪物排序）正常工作
