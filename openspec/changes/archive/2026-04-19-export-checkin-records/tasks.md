# 打卡记录导出功能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在"舰长打卡AI"设置页面添加导出打卡记录按钮，导出数据库中所有用户的打卡记录，按打卡天数排序，导出成UTF-8编码的TXT文件

**Architecture:** C++/CLI 混合架构，ProfileManager 管理用户打卡数据，通过 DataBridgeExports P/Invoke 暴露给 C# UI 层

**Tech Stack:** ["C++", "SQLite3", "P/Invoke", "WPF", "C#"]

---

## 1. C++ ProfileManager 新增方法

**Files:**
- Create: (none)
- Modify: `MonsterOrderWilds/ProfileManager.h:57-62`
- Modify: `MonsterOrderWilds/ProfileManager.cpp:658`

**精确执行序列：**

- [x] **Step 1: 修改 ProfileManager.h 添加方法声明** (2 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.h`
  2. 在第 57-62 行区域（其他方法声明之后），添加以下方法声明：

```cpp
void GetAllProfilesSortedByCheckinDays(std::vector<UserProfileData>& outProfiles);
bool ExportCheckinRecordsToFile(const std::string& filePath);
```

- [x] **Step 2: 修改 ProfileManager.cpp 实现方法** (5 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.cpp` 最后部分
  2. 在文件末尾（第 658 行之后），添加以下实现：

```cpp
void ProfileManager::GetAllProfilesSortedByCheckinDays(std::vector<UserProfileData>& outProfiles) {
    std::lock_guard<std::mutex> lock(profilesLock_);
    outProfiles.clear();
    for (const auto& pair : profiles_) {
        outProfiles.push_back(pair.second);
    }
    std::sort(outProfiles.begin(), outProfiles.end(),
        [](const UserProfileData& a, const UserProfileData& b) {
            return a.continuousDays > b.continuousDays;
        });
}

bool ProfileManager::ExportCheckinRecordsToFile(const std::string& filePath) {
    std::vector<UserProfileData> profiles;
    GetAllProfilesSortedByCheckinDays(profiles);

    std::ofstream ofs(filePath, std::ios::out | std::ios::binary);
    if (!ofs) {
        LOG_ERROR(TEXT("ProfileManager: Failed to open file for export: %hs"), filePath.c_str());
        return false;
    }

    // Write UTF-8 BOM
    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    ofs.write(reinterpret_cast<char*>(bom), 3);

    for (const auto& profile : profiles) {
        std::string username = profile.username;
        // Replace tabs in username with spaces to avoid format issues
        for (char& c : username) {
            if (c == '\t') c = ' ';
        }
        ofs << username << "\t"
            << profile.uid << "\t"
            << profile.continuousDays << "\t"
            << profile.lastCheckinDate << "\n";
    }

    ofs.close();
    LOG_INFO(TEXT("ProfileManager: Exported %d checkin records to %hs"), profiles.size(), filePath.c_str());
    return true;
}
```

- [x] **Step 3: 添加头文件引用** (1 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/ProfileManager.cpp` 第 1-10 行
  2. 在 `#include <sstream>` 之后添加 `#include <fstream>`

---

## 2. C++ DataBridgeExports 新增导出函数

**Files:**
- Create: (none)
- Modify: `MonsterOrderWilds/DataBridgeExports.h:58`
- Modify: `MonsterOrderWilds/DataBridgeExports.cpp:538`

**精确执行序列：**

- [x] **Step 1: 修改 DataBridgeExports.h 添加函数声明** (2 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/DataBridgeExports.h`
  2. 在第 58 行（`DataBridge_SetCheckinTTSPlayCallback` 声明之后，`#ifdef __cplusplus` 之前），添加：

```cpp
__declspec(dllexport) bool __stdcall ProfileManager_ExportCheckinRecords(const char* filePath);
```

- [x] **Step 2: 修改 DataBridgeExports.cpp 添加函数实现** (3 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `MonsterOrderWilds/DataBridgeExports.cpp` 最后部分
  2. 在第 538 行之后（文件末尾），添加：

```cpp
__declspec(dllexport) bool __stdcall ProfileManager_ExportCheckinRecords(const char* filePath)
{
    try
    {
        return ProfileManager::Inst()->ExportCheckinRecordsToFile(filePath);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("ProfileManager_ExportCheckinRecords failed: %s"), e.what());
        return false;
    }
}
```

---

## 3. C# NativeImports 添加 P/Invoke 声明

**Files:**
- Modify: `JonysandMHDanmuTools/NativeImports.cs:119`

**精确执行序列：**

- [x] **Step 1: 添加 P/Invoke 声明** (2 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/NativeImports.cs`
  2. 在第 97 行（`TTSManager_GetCurrentProviderName` 声明之后，`[UnmanagedFunctionPointer` 之前），添加：

```csharp
[DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
public static extern bool ProfileManager_ExportCheckinRecords(string filePath);
```

---

## 4. WPF UI 添加导出按钮

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml:623`
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs:516`

**精确执行序列：**

- [x] **Step 1: 在 ConfigWindow.xaml 添加导出按钮** (2 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ConfigWindow.xaml` 第 610-625 行
  2. 在第 617 行（`CheckinTriggerWordsTextBox` 之后），添加：

```xml
<Button
    Name="ExportCheckinRecordsButton"
    Content="导出打卡记录"
    Margin="0,16,0,0"
    Click="ExportCheckinRecordsButton_Click" />
```

- [x] **Step 2: 在 ConfigWindow.xaml.cs 添加按钮点击事件处理** (3 min)

  【工具序列】read → edit
  1. 用 read 工具读取 `JonysandMHDanmuTools/ConfigWindow.xaml.cs` 第 510-517 行
  2. 在第 515 行（`CheckinTriggerWordsTextBox_TextChanged` 方法之后），添加：

```csharp
private void ExportCheckinRecordsButton_Click(object sender, RoutedEventArgs e)
{
    var dialog = new Microsoft.Win32.SaveFileDialog
    {
        Filter = "Text files (*.txt)|*.txt",
        DefaultExt = ".txt",
        Title = "导出打卡记录"
    };

    if (dialog.ShowDialog() == true)
    {
        ExportCheckinRecordsButton.IsEnabled = false;
        ExportCheckinRecordsButton.Content = "导出中...";

        Task.Run(() =>
        {
            bool success = NativeImports.ProfileManager_ExportCheckinRecords(dialog.FileName);
            Dispatcher.Invoke(() =>
            {
                ExportCheckinRecordsButton.IsEnabled = true;
                ExportCheckinRecordsButton.Content = "导出打卡记录";

                if (success)
                {
                    System.Windows.MessageBox.Show("导出成功！", "导出打卡记录",
                        System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
                }
                else
                {
                    System.Windows.MessageBox.Show("导出失败！", "导出打卡记录",
                        System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            });
        });
    }
}
```

---

## 5. 编译验证

**Files:**
- (none - 验证步骤)

**精确执行序列：**

- [x] **Step 1: 编译项目** (2 min)

  【工具序列】bash
  执行: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
  预期: `0 个错误`

---

## Scenarios → Tasks 映射

| Scenario | Task Ref |
|----------|----------|
| Button visible in UI | Task 4.1 |
| Save dialog opens | Task 4.2 |
| User cancels dialog | Task 4.2 |
| Records sorted descending | Task 1.2 |
| TXT file content format | Task 1.2 |
| UI remains responsive during export | Task 4.2 |
| No records to export | Task 1.2 |

## 文件修改清单

| 操作 | 文件路径 |
|------|---------|
| Modify | `MonsterOrderWilds/ProfileManager.h:57-62` |
| Modify | `MonsterOrderWilds/ProfileManager.cpp:1-10,658` |
| Modify | `MonsterOrderWilds/DataBridgeExports.h:58` |
| Modify | `MonsterOrderWilds/DataBridgeExports.cpp:538` |
| Modify | `JonysandMHDanmuTools/NativeImports.cs:119` |
| Modify | `JonysandMHDanmuTools/ConfigWindow.xaml:623` |
| Modify | `JonysandMHDanmuTools/ConfigWindow.xaml.cs:516` |

---

## 执行方式选择

Tasks 已生成并保存到 `openspec/changes/export-checkin-records/tasks.md`。

两种执行方式：
1. **Subagent-Driven（推荐）** - 我为每个 Task 启动一个新的 subagent，完成后审阅再继续下一个
2. **Inline Execution** - 在当前会话中按顺序执行所有 tasks

请选择执行方式？
