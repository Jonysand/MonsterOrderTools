# Agent Rules

## 语言规则
- 所有回答、解释、注释均使用中文
- 代码中的变量名、函数名等遵循项目原有命名规范

## 代码复查规则
- **每次代码修改后必须执行 `git diff` 复查**
- 确认修改范围正确，无多余改动
- 验证改动符合任务要求

## Git 提交规则
- **永远不要自动提交（commit）和推送（push）**
- 所有提交和推送必须由用户明确要求

## 单元测试规则
- **新建 OpenSpec Change 时必须为所有修改的功能建立单元测试**
- **单元测试在代码修改后立即运行验证**
- 单元测试文件命名规范：`<模块名>Tests.cpp`
- 使用 `#ifdef RUN_UNIT_TESTS` 包裹测试代码
- 独立测试项目使用 `<模块名>Tests.vcxproj`，配置为 Console 应用
- 测试输出使用 `[PASS]` 标记，方便识别

## OpenSpec Change 规则
- **每次完成 OpenSpec Change 后，必须检查是否需要将新文件打包进安装包**
- 检查范围：
  - 新增的资源文件（图片、音频、配置文件等）
  - 新增的依赖库或组件
  - 新增的配置文件或数据文件
- 如需打包，修改 `installer/` 目录下的打包配置或 `wixproj` 文件

## 文件编码规则
- **所有源代码文件统一使用 UTF-8 with BOM 编码**
- C# 文件 (.cs): UTF-8 with BOM
- XAML 文件 (.xaml): UTF-8 with BOM
- C++ 文件 (.cpp/.h): UTF-8 with BOM
- XML 文件 (.vcxproj/.csproj): UTF-8 无 BOM
- JSON/YAML 配置文件: UTF-8

## C++ 项目规则

### 头文件引用
- 必须优先引用 `framework.h`
- 示例：`#include "framework.h"` 应放在第一行

### vcxproj 文件组织
- Header Files 和 Source Files 按**字母顺序**排列
- 新增文件必须按正确位置插入
- Debug 专用文件需添加条件排除：
  ```xml
  <ClCompile Include="MimoTTSClientTests.cpp">
    <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
  </ClCompile>
  ```
- **新增文件时必须同时添加到对应的 Filter 中**，否则文件不会显示在 Visual Studio 的虚拟文件夹里
- 添加文件后检查 `.vcxproj.filters` 文件，确保新增文件在正确的 `<Filter>` 节点下

### 编译宏
- RUN_UNIT_TESTS: Debug 模式启用，用于单元测试

### 调试日志规则
- **调试日志一律使用 `LOG_DEBUG`**
- `LOG_DEBUG` 在 Debug 配置下启用，在 Release 配置下为空宏（不输出）
- 临时添加的调试日志使用 `LOG_DEBUG`，不要使用 `LOG_INFO` 或 `LOG_ERROR`
- 示例：`LOG_DEBUG(TEXT("ConfigManager: LoadConfig - idCode from registry: [%s]"), config_.idCode.c_str());`

### Filter（虚拟文件夹）
- Filter 配置放在 `.vcxproj.filters` 文件中
- 单元测试文件放在 `UnitTests` Filter 下
- vcxproj 中添加条件排除：
  ```xml
  <ClCompile Include="MimoTTSClientTests.cpp">
    <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
  </ClCompile>
  ```

## 配置字段扩展规则

新增配置字段时，需要同时修改以下文件：

### 1. C++ 层（持久化层）

| 文件 | 修改内容 |
|------|---------|
| `MonsterOrderWilds/ConfigManager.h` | 在 `ConfigData` 结构体中添加字段 |
| `MonsterOrderWilds/ConfigFieldRegistry.cpp` | 在 `RegisterAll()` 中注册字段 |
| `MonsterOrderWilds/ConfigManager.cpp` | 在 `LoadConfig()` 和 `SaveConfig()` 中处理 JSON |
| `MonsterOrderWilds/DataBridgeWrapper.h` | 在 `ConfigProxy` 类的 `Refresh()`、`Apply()` 和属性中添加 |

### 2. C# 层（应用层）

| 文件 | 修改内容 |
|------|---------|
| `JonysandMHDanmuTools/DataStructures.cs` | 在 `ConfigDataSnapshot` 结构体中添加字段，并在 `FromMainConfig()` 和 `ApplyTo()` 中处理 |
| `JonysandMHDanmuTools/Utils.cs` | 在 `ConfigFieldRegistry` 静态构造函数中注册字段，在 `MainConfig` 类中添加属性 |
| `JonysandMHDanmuTools/ProxyClasses.cs` | 在 `ConfigProxy` 类中添加属性，并在 `RefreshFromConfig()` 和 `ApplyToConfig()` 中处理 |
| `JonysandMHDanmuTools/ToolsMain.cs` | 在 `ConfigChanged()` 方法中添加处理逻辑（如果需要响应配置变更） |

### 3. UI 层（可选）

如果配置需要用户界面输入，还需要修改：
- `JonysandMHDanmuTools/ConfigWindow.xaml` - 添加 UI 控件
- `JonysandMHDanmuTools/ConfigWindow.xaml.cs` - 在 `FillConfig()` 中初始化控件，处理控件事件

### 字段命名规范

| 层 | 命名风格 | 示例 |
|----|---------|------|
| C++ ConfigData | camelCase | `defaultMarqueeText` |
| JSON 配置 | SCREAMING_SNAKE | `DEFAULT_MARQUEE_TEXT` |
| C# MainConfig | SCREAMING_SNAKE | `DEFAULT_MARQUEE_TEXT` |
| UI 控件 Name | PascalCase | `DefaultMarqueeTextBox` |

### 注意事项

- **必须同时修改 C++ 和 C# 两层**，否则运行时会出现 `Config field 'xxx' not found` 错误
- C++ 层的 `ConfigFieldRegistry` 和 C# 层的 `ConfigFieldRegistry` 是两个独立的注册表，需要分别注册
- 如果字段需要即时生效，需要在 `ToolsMain.ConfigChanged()` 中添加处理逻辑
- **如果配置变更需要通知其他组件（如 UI 刷新），必须在 `ToolsMain.ConfigChanged()` 中调用 `GlobalEventListener.Invoke("事件名", 值)` 发送通知**，并在该组件中注册监听此事件

## 编译验证规则

**MSBuild 编译命令**（后续使用此简写方式执行编译验证）：
```
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

**注意**：由于 PowerShell 版本限制，不能使用 `&&` 连接命令。编译完成后检查输出中的 `0 个错误` 确认编译成功。

<!-- OCR:START -->
## Open Code Review Instructions

These instructions are for AI assistants handling code review in this project.

Always open `.ocr/skills/SKILL.md` when the request:
- Asks for code review, PR review, or feedback on changes
- Mentions "review my code" or similar phrases
- Wants multi-perspective analysis of code quality
- Asks to map, organize, or navigate a large changeset

Use `.ocr/skills/SKILL.md` to learn:
- How to run the 8-phase review workflow
- How to generate a Code Review Map for large changesets
- Available reviewer personas and their focus areas
- Session management and output format

Keep this managed block so `ocr init` can refresh the instructions.

<!-- OCR:END -->
