# Agent Rules

## 语言规则
- 所有回答、解释、注释均使用中文
- 代码中的变量名、函数名等遵循项目原有命名规范

## 代码复查规则
- **每次代码修改后必须执行 `git diff` 复查**
- 确认修改范围正确，无多余改动
- 验证改动符合任务要求

## 单元测试规则
- **新建 OpenSpec Change 时必须为所有修改的功能建立单元测试并运行测试**
- 单元测试文件命名规范：`<模块名>Tests.cpp`
- 使用 `#ifdef RUN_UNIT_TESTS` 包裹测试代码
- 独立测试项目使用 `<模块名>Tests.vcxproj`，配置为 Console 应用
- 测试输出使用 `[PASS]` 标记，方便识别

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

### 编译宏
- USE_MIMO_TTS: 编译时开关，默认启用（设为0可排除MiMo代码）
- RUN_UNIT_TESTS: Debug 模式启用，用于单元测试

### Filter（虚拟文件夹）
- Filter 配置放在 `.vcxproj.filters` 文件中
- 单元测试文件放在 `UnitTests` Filter 下
- vcxproj 中添加条件排除：
  ```xml
  <ClCompile Include="MimoTTSClientTests.cpp">
    <ExcludedFromBuild Condition="'$(Configuration)'=='Release'">true</ExcludedFromBuild>
  </ClCompile>
  ```
