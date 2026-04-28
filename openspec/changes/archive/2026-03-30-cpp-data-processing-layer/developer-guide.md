# 开发指南与故障排除

## 开发环境搭建

### 前置条件
- Visual Studio 2022 (Community 或更高)
- Windows SDK (最新版)
- .NET Framework 4.7.2+

### 项目结构
```
JonysandMHDanmuTools/
├── MonsterOrderWilds/           # C++ 项目
│   ├── *.h / *.cpp              # 源代码
│   ├── *Tests.cpp               # 单元测试（#ifdef RUN_UNIT_TESTS）
│   └── MonsterOrderWilds.vcxproj
├── JonysandMHDanmuTools/        # C# 项目
│   ├── *.cs                     # 源代码
│   ├── *.xaml                   # WPF界面
│   └── JonysandMHDanmuTools.csproj
└── openspec/                    # OpenSpec变更管理
    └── changes/
```

### 编译步骤
1. 打开 `JonysandMHDanmuTools.sln`
2. 选择 `Release | x64` 配置
3. 生成解决方案

### 编译命令（命令行）
```bash
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "JonysandMHDanmuTools.sln" -p:Configuration=Release -p:Platform=x64 -t:Build -m
```

## 编码规范

### C++ 文件
- **编码**: UTF-8 with BOM
- **头文件**: `#include "framework.h"` 必须放在第一行
- **单例**: 使用 `DECLARE_SINGLETON` / `DEFINE_SINGLETON` 宏
- **线程安全**: 使用项目自定义的 `Lock` 类（spinlock）

### C# 文件
- **编码**: UTF-8 with BOM
- **命名空间**: `MonsterOrderWindows`
- **UI线程**: 使用 `Dispatcher.InvokeAsync`

### vcxproj 文件
- **编码**: UTF-8 无 BOM
- **文件排列**: Header Files 和 Source Files 按字母顺序
- **Debug专用文件**: 添加条件排除
  ```xml
  <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
  ```

## 单元测试

### 运行测试
1. 切换到 `Debug | x64` 配置
2. 添加 `RUN_UNIT_TESTS` 预处理器定义
3. 编译运行，控制台输出 `[PASS]` 标记

### 测试文件列表
| 文件 | 模块 | 测试数 |
|------|------|--------|
| ConfigManagerTests.cpp | 配置管理 | 8 |
| MonsterDataManagerTests.cpp | 怪物匹配 | 10 |
| PriorityQueueManagerTests.cpp | 队列操作 | 6 |
| DanmuProcessorTests.cpp | 弹幕处理 | 4 |
| StringProcessorTests.cpp | 字符串处理 | 10 |
| DataBridgeTests.cpp | 桥接接口 | 5 |
| MimoTTSClientTests.cpp | MiMo TTS | 5 |

### 编写新测试
```cpp
#include "framework.h"
#include "YourModule.h"
#include <iostream>
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestYourFeature()
{
    // 测试逻辑
    assert(condition);

    std::cout << "[PASS] TestYourFeature" << std::endl;
}

void RunAllYourModuleTests()
{
    std::cout << "=== YourModule Tests ===" << std::endl;
    TestYourFeature();
    std::cout << "=== YourModule Tests Done ===" << std::endl;
}

#endif // RUN_UNIT_TESTS
```

## 常见问题

### 编译错误

**Q: `nlohmann/json.hpp` not found**
- A: LSP 假阳性。实际 MSBuild 编译使用正确的 include 路径。

**Q: `ToolsMainHost` undeclared**
- A: 需要在 Debug 配置下编译，且需要 `PluginAppHost.h` 头文件。

**Q: `System::String^` 相关错误**
- A: C++/CLI 代码需要 `/clr` 编译选项，检查项目配置。

### 运行时问题

**Q: 配置文件丢失**
- A: 程序会在 `MonsterOrderWilds_configs/` 目录下创建默认配置。

**Q: TTS 无声音**
- A: 检查 `ENABLE_VOICE` 是否开启，检查 TTS 引擎配置。

**Q: 怪物列表不匹配**
- A: 确保 `MonsterOrderWilds_configs/monster_list.json` 格式正确且编码为 UTF-8。

**Q: 内存泄漏**
- A: 检查 `ConvertToTCHAR` 返回的 TString 是否正确使用（已修复为值类型返回）。

### 性能问题

**Q: UI 卡顿**
- A: 确保使用 `Dispatcher.InvokeAsync` 而非 `Dispatcher.Invoke`。
- A: 检查是否有文件 I/O 操作在 UI 线程执行。

**Q: TTS 播报延迟**
- A: 检查 TTS 异步队列状态，确保没有阻塞的同步等待。

## 调试技巧

### 启用日志
日志通过 `WriteLog.h` 中的宏输出：
- `LOG_INFO()` - 信息
- `LOG_WARNING()` - 警告
- `LOG_ERROR()` - 错误

### 查看事件流
在 GlobalEventListener 中注册回调来跟踪事件：
```csharp
GlobalEventListener.AddListener("ConfigChanged", (msg) => 
    Console.WriteLine($"Config: {msg}"));
```

### 内存分析
使用 Visual Studio 诊断工具（调试模式）：
1. 调试 → 性能分析器
2. 选择 ".NET 内存分配" 或 "本机内存"
3. 开始分析

## Git 工作流

### 提交规范
- `feat:` 新功能
- `fix:` 修复
- `refactor:` 重构
- `test:` 测试
- `docs:` 文档

### OpenSpec 变更
```bash
# 查看变更状态
openspec status --change "change-name" --json

# 实施变更
/opsx-apply change-name

# 归档变更
/opsx-archive change-name
```
