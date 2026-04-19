## Context

当前系统使用 C++/CLI 混合架构：
- `MonsterOrderWilds/`: C++ 核心层，包含 ProfileManager 负责管理用户打卡数据
- `JonysandMHDanmuTools/`: C# WPF UI 层，通过 P/Invoke 与 C++ 层通信

数据库（captain_profiles.db）存储用户资料，包含 `uid`、`username`、`continuousDays`、`lastCheckinDate` 等字段。ProfileManager 已实现 SaveProfile、LoadProfile、RecordCheckin 等方法，但缺少获取所有用户资料的方法。

## Goals / Non-Goals

**Goals:**
- 实现导出打卡记录功能，按打卡天数降序排列
- 在"舰长打卡AI"标签页添加导出按钮
- 导出为 UTF-8 编码的 TXT 文件
- 导出操作不阻塞 UI

**Non-Goals:**
- 不修改现有打卡功能逻辑
- 不支持其他导出格式（CSV、JSON 等）
- 不支持按其他字段排序

## Decisions

### 1. C++ 层新增方法获取所有用户资料

**Decision:** 在 ProfileManager 中新增 `GetAllProfilesSortedByCheckinDays` 方法，返回按 `continuousDays` 降序排列的所有用户资料。

**Rationale:** 直接在 C++ 层排序，避免在 C# 层处理大量数据。

**Alternatives:**
- 在 C# 层排序：增加跨边界数据传输量，性能较差
- 导出到数据库同目录的固定文件：用户无法选择保存位置，体验差

### 2. 通过 P/Invoke 暴露导出函数

**Decision:** 在 DataBridgeExports 中新增 `ProfileManager_ExportCheckinRecords` 函数，签名：
```cpp
bool ProfileManager_ExportCheckinRecords(const char* filePath)
```

**Rationale:** 保持与现有 P/Invoke 模式一致，现有功能如 Config_GetString、PriorityQueue_* 等都采用此模式。

### 3. C# 层使用 SaveFileDialog

**Decision:** 使用 WPF 的 SaveFileDialog 让用户选择保存路径。

**Rationale:** WPF 原生支持，代码简洁，用户体验好。

### 4. 后台线程执行导出

**Decision:** 使用 `Task.Run()` 在后台线程执行导出操作，完成后通过 Dispatcher 更新 UI。

**Rationale:** 避免大数据库导出时 UI 卡顿，保持应用响应性。

## Risks / Trade-offs

**[Risk]** 数据库文件被其他进程锁定
→ **Mitigation:** 导出操作使用只读访问，避免写入冲突

**[Risk]** 用户取消导出时已部分写入文件
→ **Mitigation:** 先写入临时文件，操作完成后 rename，避免文件损坏

**[Risk]** 用户名包含制表符导致格式错乱
→ **Mitigation:** 将用户名中的制表符替换为空格，或对内容进行转义
