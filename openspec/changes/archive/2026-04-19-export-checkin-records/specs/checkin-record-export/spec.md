## ADDED Requirements

### Requirement: Export Checkin Records Button
系统 SHALL 在"舰长打卡AI"设置标签页中添加"导出打卡记录"按钮。

#### Scenario: Button visible in UI
- **WHEN** 用户打开"舰长打卡AI"标签页
- **THEN** "导出打卡记录"按钮可见且可点击

### Requirement: Export File Dialog
点击导出按钮后，系统 SHALL 弹出文件保存对话框让用户选择保存路径。

#### Scenario: Save dialog opens
- **WHEN** 用户点击"导出打卡记录"按钮
- **THEN** 系统显示保存文件对话框，过滤器设置为 TXT 文件（*.txt）

#### Scenario: User cancels dialog
- **WHEN** 用户在保存对话框中点击"取消"
- **THEN** 不执行任何导出操作，对话框关闭

### Requirement: Export Sorted by Checkin Days
导出的打卡记录 SHALL 按打卡天数（continuousDays）降序排列。

#### Scenario: Records sorted descending
- **WHEN** 用户成功导出打卡记录
- **THEN** 第一条记录的打卡天数大于等于后续所有记录

### Requirement: Export Format
导出文件 SHALL 使用 UTF-8 编码，每行格式为：`用户名\tUID\t累计打卡天数\t最后打卡日期`

#### Scenario: TXT file content format
- **WHEN** 用户导出打卡记录到文件
- **THEN** 文件每行包含制表符分隔的用户名、UID、累计打卡天数、最后打卡日期

### Requirement: Background Execution
导出操作 SHALL 在后台线程执行，避免阻塞 UI 线程。

#### Scenario: UI remains responsive during export
- **WHEN** 用户点击"导出打卡记录"按钮
- **THEN** UI 保持响应，导出完成后显示成功或失败提示

### Requirement: Empty Database Handling
当数据库中没有打卡记录时，系统 SHALL 导出空文件或提示"暂无打卡记录"。

#### Scenario: No records to export
- **WHEN** 数据库中没有用户打卡记录
- **THEN** 系统导出空文件或显示"暂无打卡记录"提示
