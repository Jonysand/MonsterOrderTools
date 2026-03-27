# Agent Rules

## 语言规则
- 所有回答、解释、注释均使用中文
- 代码中的变量名、函数名等遵循项目原有命名规范

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
