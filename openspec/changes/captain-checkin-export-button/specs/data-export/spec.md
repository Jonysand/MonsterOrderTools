## ADDED Requirements

### Requirement: Export captain check-in records
系统应允许用户导出舰长打卡记录到CSV或JSON文件，支持按用户筛选和时间范围筛选。

#### Scenario: Successful export to CSV
- **WHEN** 用户点击"导出打卡记录"按钮，选择CSV格式，不设置筛选条件
- **THEN** 系统生成包含所有用户打卡记录的CSV文件，文件包含列：用户ID、用户名、打卡日期、连续天数、累计天数、创建时间

#### Scenario: Successful export to JSON
- **WHEN** 用户点击"导出打卡记录"按钮，选择JSON格式，不设置筛选条件
- **THEN** 系统生成包含所有用户打卡记录的JSON文件，数据结构为数组，每条记录包含字段：uid、username、checkinDate、continuousDays、cumulativeDays、createdAt

#### Scenario: Export for specific user
- **WHEN** 用户点击"导出打卡记录"按钮，选择特定用户（通过用户名或UID筛选）
- **THEN** 系统仅导出该用户的打卡记录，格式符合用户选择的CSV或JSON

#### Scenario: Export with date range filter
- **WHEN** 用户设置开始日期和结束日期，点击导出
- **THEN** 系统仅导出指定日期范围内的打卡记录

#### Scenario: Export empty dataset
- **WHEN** 数据库中没有符合条件的打卡记录
- **THEN** 系统生成空文件（CSV只有表头，JSON为空数组），并提示用户"没有找到符合条件的打卡记录"

#### Scenario: Export failure due to file permission
- **WHEN** 用户选择的保存路径没有写入权限
- **THEN** 系统显示错误信息"无法写入文件，请检查文件权限"，不生成文件

#### Scenario: Export failure due to database error
- **WHEN** 数据库查询过程中发生错误
- **THEN** 系统显示错误信息"导出失败，数据库错误"，不生成文件