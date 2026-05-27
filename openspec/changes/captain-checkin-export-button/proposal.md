## Why

用户需要导出舰长打卡记录用于数据分析、备份或分享。目前打卡记录仅存储在本地SQLite数据库中，没有便捷的导出途径。添加导出按钮可以简化操作流程，提升用户体验。

## What Changes

- 在舰长打卡AI配置页面添加"导出打卡记录"按钮
- 实现导出功能，支持将打卡记录导出为CSV或JSON格式
- 导出范围可配置：单个用户或所有用户
- 导出内容包括：用户ID、用户名、打卡日期、连续天数、累计天数等

## Capabilities

### New Capabilities
- `data-export`: 导出舰长打卡记录到文件，支持CSV/JSON格式，可筛选用户和时间范围
- `ui-config-window`: 在舰长打卡AI配置页添加导出按钮

### Modified Capabilities
<!-- 无修改的能力 -->

## Impact

- **C# UI层**：`ConfigWindow.xaml`添加按钮，`ConfigWindow.xaml.cs`添加点击事件处理
- **C++后层**：可能需要新增导出API或扩展现有`ProfileManager`的查询功能
- **数据格式**：定义导出数据结构，确保UTF-8编码兼容中文
- **文件保存**：使用Windows文件对话框选择保存路径
- **依赖**：无新增外部依赖