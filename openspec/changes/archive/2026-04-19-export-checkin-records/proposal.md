## Why

当前程序没有提供导出舰长打卡记录的功能。用户需要将所有用户的打卡数据导出到本地文件进行分析或备份。现有数据库（captain_profiles.db）已存储了完整的打卡信息，但缺乏便捷的导出途径。

## What Changes

- 在"舰长打卡AI"设置页面添加"导出打卡记录"按钮
- 点击按钮后弹出文件保存对话框，允许用户选择导出路径和文件名
- 导出文件格式为 UTF-8 编码的 TXT 文件
- 导出内容按打卡天数（continuousDays）降序排列
- 每行格式：`用户名\tUID\t累计打卡天数\t最后打卡日期`
- 导出操作在后台线程执行，避免阻塞 UI

## Capabilities

### New Capabilities
- `checkin-record-export`: 提供舰长打卡记录的导出功能，支持按打卡天数排序并保存为 TXT 文件

### Modified Capabilities
<!-- 暂无修改现有功能 -->

## Impact

**受影响的代码：**
- `MonsterOrderWilds/ProfileManager.h/cpp`: 新增 `GetAllProfiles` 方法用于获取所有用户资料
- `MonsterOrderWilds/DataBridgeExports.h/cpp`: 新增导出相关的 P/Invoke 函数
- `JonysandMHDanmuTools/NativeImports.cs`: 新增 C# 端 P/Invoke 声明
- `JonysandMHDanmuTools/ConfigWindow.xaml`: 在"舰长打卡AI"标签页添加导出按钮
- `JonysandMHDanmuTools/ConfigWindow.xaml.cs`: 添加按钮点击事件处理
