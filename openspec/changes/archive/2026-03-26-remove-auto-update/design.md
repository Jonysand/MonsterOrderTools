## Context

弹幕姬插件当前包含自动更新功能，通过 HTTPS 请求与外部服务器通信获取更新。该功能由 `AppUpdater` 类实现，包括 app 版本检查、文件下载、怪物列表更新等。由于服务器可能不再维护，需要移除此功能以避免潜在的网络请求失败和维护负担。

技术栈：C++ (Win32/WinHTTP) + C# WPF (.NET Framework)

## Goals / Non-Goals

**Goals:**
- 完全移除自动更新功能，包括 UI、命令处理、网络请求
- 清理所有相关代码和依赖，保持代码库整洁
- 不影响核心功能（弹幕处理、怪物排序、优先级队列）

**Non-Goals:**
- 不实现替代的更新机制（用户需手动下载更新）
- 不修改核心业务逻辑
- 不重构其他网络相关功能（如弹幕接收）

## Decisions

### Decision 1: 删除整个 AppUpdater 模块

**选择**: 删除 `AppUpdater.cpp` 和 `AppUpdater.h` 两个文件。

**理由**: 该模块的所有功能（app 更新、怪物列表更新）都不再需要。保留空壳代码会增加维护负担。

**替代方案考虑**:
- 保留文件但注释掉实现：不彻底，容易混淆
- 保留类结构但移除方法调用：无意义，类不会被使用

### Decision 2: 移除 JONYSAND_URL 常量

**选择**: 从 `Network.h` 中删除 `JONYSAND_URL` 常量定义。

**理由**: 经代码搜索确认，此常量仅在 `AppUpdater.cpp` 中使用。删除 AppUpdater 后，此常量成为死代码。

**风险**: 如果未来有其他功能需要连接同一服务器，需要重新添加。但当前无此计划。

### Decision 3: 保留 Network 模块的其他功能

**选择**: 保留 `Network.h` 和 `Network.cpp` 中的 `MakeHttpsRequest` 等通用网络函数。

**理由**: 这些函数可能被其他功能使用，或未来扩展。仅移除与更新相关的特定调用。

### Decision 4: 在 MonsterOrderWilds.cpp 中移除 AppUpdater 调用点

**选择**: 删除所有 `AppUpdater::Inst()` 调用，包括初始化、Tick、命令处理。

**理由**: 这些调用会链接到已删除的 AppUpdater 类，导致编译错误。

## Risks / Trade-offs

- **[用户无法自动更新]** → 用户需要手动访问发布页面下载新版本。这是有意的行为变更，需要在 release notes 中说明。
- **[移除 JONYSAND_URL 可能影响未来功能]** → 如果未来需要连接同一服务器，需要重新添加。当前风险较低。
- **[删除文件可能导致构建配置问题]** → 需要检查项目文件（.vcxproj）是否引用了 AppUpdater.cpp/h，如有需同步删除引用。

## Migration Plan

1. 删除 `AppUpdater.cpp` 和 `AppUpdater.h`
2. 从 `MonsterOrderWilds.cpp` 中移除所有 AppUpdater 相关代码
3. 从 `ConfigWindow.xaml` 中移除更新按钮
4. 从 `ConfigWindow.xaml.cs` 中移除按钮事件处理
5. 从 `Network.h` 中移除 `JONYSAND_URL` 常量
6. 检查并清理项目文件（.vcxproj）中的引用
7. 编译验证无错误
8. 运行测试确认核心功能正常
