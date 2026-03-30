## REMOVED Requirements

### Requirement: Auto-update app from remote server
**Reason**: 服务器可能不再维护，自动更新功能可能导致网络请求失败或安全隐患
**Migration**: 用户需要手动访问发布页面下载新版本并替换文件

#### Scenario: Check app version
- **WHEN** 用户点击"更新app"按钮
- **THEN** 系统不再发送版本检查请求

#### Scenario: Download app update
- **WHEN** 服务器返回更高版本号
- **THEN** 系统不再下载更新文件

#### Scenario: Apply app update
- **WHEN** 所有更新文件下载完成
- **THEN** 系统不再创建更新标记并重启

### Requirement: Auto-update monster list from remote server
**Reason**: 服务器可能不再维护，且怪物列表更新应由用户手动控制
**Migration**: 用户需要手动下载最新的 monster_list.json 文件并替换

#### Scenario: Check monster list version
- **WHEN** 用户点击"更新怪物列表"按钮
- **THEN** 系统不再发送版本检查请求

#### Scenario: Download monster list
- **WHEN** 服务器返回更高版本号
- **THEN** 系统不再下载怪物列表文件

#### Scenario: Apply monster list update
- **WHEN** 怪物列表下载完成
- **THEN** 系统不再保存文件并刷新内存数据

### Requirement: Update UI buttons in config window
**Reason**: 移除更新功能后，对应的 UI 按钮不再需要

#### Scenario: Config window display
- **WHEN** 用户打开配置窗口
- **THEN** 窗口不再显示"更新app"和"更新怪物列表"按钮
