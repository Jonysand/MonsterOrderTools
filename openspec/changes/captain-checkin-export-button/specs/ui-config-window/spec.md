## ADDED Requirements

### Requirement: Add export button to captain check-in AI configuration page
系统应在舰长打卡AI配置页添加"导出打卡记录"按钮，点击后触发导出功能。

#### Scenario: Export button is visible
- **WHEN** 用户打开舰长打卡AI配置页
- **THEN** 页面上显示"导出打卡记录"按钮，位置合理，样式与其他按钮一致

#### Scenario: Click export button opens export options
- **WHEN** 用户点击"导出打卡记录"按钮
- **THEN** 系统显示导出选项对话框，允许用户选择导出格式（CSV/JSON）、筛选用户和日期范围

#### Scenario: Export button disabled when no data
- **WHEN** 数据库中没有任何打卡记录
- **THEN** "导出打卡记录"按钮处于禁用状态，鼠标悬停提示"没有可导出的打卡记录"

#### Scenario: Export button enabled when data exists
- **WHEN** 数据库中存在打卡记录
- **THEN** "导出打卡记录"按钮处于启用状态