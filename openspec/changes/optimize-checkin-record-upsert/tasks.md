# Checkin Record Upsert Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 checkin_records 表的打卡记录操作从 INSERT OR IGNORE 改为 INSERT ... ON CONFLICT DO UPDATE 逻辑

**Architecture:** 使用 SQLite ON CONFLICT DO UPDATE 子句实现 upsert 行为，确保新用户 INSERT、老用户当日第一次打卡 UPDATE、老用户当日重复打卡 IGNORE

**Tech Stack:** ["C++", "SQLite3", "ProfileManager.cpp"]

---

## 1. 修改 InsertCheckinRecordWithRetry 函数

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.cpp:26-68`

**精确执行序列：**

- [ ] **Step 1: 修改 SQL 语句** (2-5 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.cpp:26-68`
  2. 用 edit 工具将 `INSERT OR IGNORE` 改为 `INSERT ... ON CONFLICT DO UPDATE`

  将：
  ```cpp
  const char* sql = "INSERT OR IGNORE INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)";
  ```

  改为：
  ```cpp
  const char* sql = "INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?) ON CONFLICT(uid, checkin_date) DO UPDATE SET created_at = excluded.created_at, username = excluded.username WHERE checkin_records.created_at != excluded.created_at";
  ```

- [ ] **Step 2: 验证编译** (1 min)

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

---

## 2. 修改 RecordCheckin 函数

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.cpp:337-385`

**精确执行序列：**

- [ ] **Step 1: 读取 RecordCheckin 函数** (1 min)

  【工具序列】read
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.cpp:337-385`

- [ ] **Step 2: 修改 RecordCheckin 中的 SQL 语句** (2 min)

  【工具序列】edit
  1. 用 edit 工具将 `RecordCheckin` 函数中的 SQL 从 `INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?)` 改为 `INSERT INTO checkin_records (uid, checkin_date, created_at, username) VALUES (?, ?, ?, ?) ON CONFLICT(uid, checkin_date) DO UPDATE SET created_at = excluded.created_at, username = excluded.username WHERE checkin_records.created_at != excluded.created_at`

- [ ] **Step 3: 验证编译** (1 min)

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

---

## 3. 修改 RecordCheckinAsync 函数

**Files:**
- Modify: `MonsterOrderWilds/ProfileManager.cpp:387-450`

**精确执行序列：**

- [ ] **Step 1: 读取 RecordCheckinAsync 函数** (1 min)

  【工具序列】read
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.cpp:387-450`

- [ ] **Step 2: 修改 RecordCheckinAsync 中的 SQL 语句** (2 min)

  【工具序列】edit
  1. 用 edit 工具将 `RecordCheckinAsync` 函数中调用的 `InsertCheckinRecordWithRetry` 保持不变，因为该函数已在 Task 1 中修改

- [ ] **Step 3: 验证编译** (1 min)

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

---

## 4. 更新单元测试

**Files:**
- Modify: `MonsterOrderWilds/CaptainCheckInModuleTests.cpp`

**精确执行序列：**

- [ ] **Step 1: 读取现有测试** (1 min)

  【工具序列】read
  1. 用 read 工具读取 `MonsterOrderWilds/CaptainCheckInModuleTests.cpp`

- [ ] **Step 2: 添加 Upsert 行为测试** (5 min)

  【工具序列】read → edit
  1. 用 read 工具读取现有测试结构
  2. 用 edit 工具添加测试用例验证 upsert 行为：
     - 新用户第一次打卡：INSERT 成功
     - 老用户同日重复打卡：UPDATE 触发（时间戳更新）
     - 老用户同日时间戳相同时打卡：IGNORE

- [ ] **Step 3: 运行测试验证** (1 min)

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

---

## JSON 格式任务映射

```json
{
  "tasks_from_scenarios": [
    {"scenario_id": 1, "scenario": "新用户第一次打卡 → INSERT", "task_ref": "Task 1"},
    {"scenario_id": 2, "scenario": "老用户在不同日期打卡 → INSERT", "task_ref": "Task 1"},
    {"scenario_id": 3, "scenario": "老用户在同一日期重复打卡 → UPDATE", "task_ref": "Task 1"},
    {"scenario_id": 4, "scenario": "老用户同一日期时间戳相同 → IGNORE", "task_ref": "Task 1"}
  ],
  "files_to_create": [],
  "files_to_modify": [
    "MonsterOrderWilds/ProfileManager.cpp:26-68",
    "MonsterOrderWilds/ProfileManager.cpp:337-385",
    "MonsterOrderWilds/ProfileManager.cpp:387-450",
    "MonsterOrderWilds/CaptainCheckInModuleTests.cpp"
  ]
}
```

---

## 覆盖情况验证

| Scenario | Task | 覆盖状态 |
|----------|------|---------|
| 新用户第一次打卡 → INSERT | Task 1 | [x] |
| 老用户在不同日期打卡 → INSERT | Task 1 | [x] |
| 老用户在同一日期重复打卡 → UPDATE | Task 1 | [x] |
| 老用户同一日期时间戳相同 → IGNORE | Task 1 | [x] |
