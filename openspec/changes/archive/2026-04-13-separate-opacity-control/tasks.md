# Separate Opacity Control Implementation Plan

**Goal:** 用户可以分别设置非穿透模式和穿透模式的窗口透明度

**Architecture:** 在现有配置系统中新增 `penetratingModeOpacity` 字段，与 `opacity` 并列。UI 新增独立滑块，业务逻辑根据窗口锁定状态选择对应透明度值。

**Tech Stack:** C++ (ConfigManager), C# (WPF/XAML)

---

## Task 1: C++ ConfigData 新增字段

**Files:**
- Modify: `MonsterOrderWilds/ConfigManager.h:20` (在 `opacity` 字段后添加)
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp:56` (注册新字段)
- Modify: `MonsterOrderWilds/ConfigManager.cpp:98` (LoadConfig 读取)
- Modify: `MonsterOrderWilds/ConfigManager.cpp:170` (SaveConfig 保存)

- [ ] **Step 1: 在 ConfigData 结构体中添加 penetratingModeOpacity 字段**

打开 `ConfigManager.h`，在 `int opacity = 100;` 后添加：
```cpp
int penetratingModeOpacity = 50;
```

- [ ] **Step 2: 在 ConfigFieldRegistry.cpp 中注册新字段**

在 `REGISTER_FIELD("opacity", ...` 后添加：
```cpp
REGISTER_FIELD("penetratingModeOpacity", int, penetratingModeOpacity, ConfigFieldType::Int);
```

- [ ] **Step 3: 在 LoadConfig 中读取 penetratingModeOpacity**

在 `ConfigManager.cpp:98` 后添加：
```cpp
if (j.contains("PENETRATING_MODE_OPACITY")) config_.penetratingModeOpacity = j["PENETRATING_MODE_OPACITY"].get<int>();
```

- [ ] **Step 4: 在 SaveConfig 中保存 penetratingModeOpacity**

在 `ConfigManager.cpp:170` 后添加：
```cpp
j["PENETRATING_MODE_OPACITY"] = config_.penetratingModeOpacity;
```

- [ ] **Step 5: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 2: DataBridgeWrapper 新增属性

**Files:**
- Modify: `MonsterOrderWilds/DataBridgeWrapper.h:29` (添加属性)
- Modify: `MonsterOrderWilds/DataBridgeWrapper.h:58` (添加 data 赋值)
- Modify: `MonsterOrderWilds/DataBridgeWrapper.h:94` (添加 property 声明)

- [ ] **Step 1: 在 ConfigProxy 中添加 PenetratingModeOpacity 属性**

在 `DataBridgeWrapper.h:29` 附近找到 `Opacity = config.opacity;`，在其后添加：
```cpp
PenetratingModeOpacity = config.penetratingModeOpacity;
```

- [ ] **Step 2: 在 SetConfigData 中添加 data 赋值**

在 `DataBridgeWrapper.h:58` 附近找到 `data.opacity = Opacity;`，在其后添加：
```cpp
data.penetratingModeOpacity = PenetratingModeOpacity;
```

- [ ] **Step 3: 添加 property 声明**

在 `DataBridgeWrapper.h:94` 附近，在 `property int Opacity;` 后添加：
```cpp
property int PenetratingModeOpacity;
```

- [ ] **Step 4: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 3: C# DataStructures 新增字段

**Files:**
- Modify: `JonysandMHDanmuTools/DataStructures.cs:21` (添加字段)
- Modify: `JonysandMHDanmuTools/DataStructures.cs:63` (FromMainConfig)
- Modify: `JonysandMHDanmuTools/DataStructures.cs:95` (ApplyTo)

- [ ] **Step 1: 在 ConfigDataSnapshot 中添加 PenetratingModeOpacity 字段**

在 `DataStructures.cs:21` 附近，在 `public int Opacity;` 后添加：
```csharp
public int PenetratingModeOpacity;
```

- [ ] **Step 2: 在 FromMainConfig 中读取值**

在 `DataStructures.cs:63` 附近，在 `Opacity = config.OPACITY,` 后添加：
```csharp
PenetratingModeOpacity = config.PENETRATING_MODE_OPACITY,
```

- [ ] **Step 3: 在 ApplyTo 中写入值**

在 `DataStructures.cs:95` 附近，在 `config.OPACITY = Opacity;` 后添加：
```csharp
config.PENETRATING_MODE_OPACITY = PenetratingModeOpacity;
```

- [ ] **Step 4: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 4: C# Utils 注册字段和属性

**Files:**
- Modify: `JonysandMHDanmuTools/Utils.cs:183-185` (注册新字段)
- Modify: `JonysandMHDanmuTools/Utils.cs:364-368` (MainConfig 属性)

- [ ] **Step 1: 在 ConfigFieldRegistry 静态构造函数中注册 penetratingModeOpacity**

在 `Utils.cs:183-185` 附近，在 `Register("opacity", ...)` 后添加：
```csharp
Register("penetratingModeOpacity", ConfigFieldType.Int,
    () => GetInt("penetratingModeOpacity"),
    v => SetValue("penetratingModeOpacity", (int)v, ConfigFieldType.Int));
```

- [ ] **Step 2: 在 MainConfig 中添加 PENETRATING_MODE_OPACITY 属性**

在 `Utils.cs:364-368` 附近，在 `public int OPACITY` 属性后添加：
```csharp
public int PENETRATING_MODE_OPACITY
{
    get => (int)ConfigFieldRegistry.Get("penetratingModeOpacity");
    set { ConfigFieldRegistry.Set("penetratingModeOpacity", value); OnPropertyChanged(); }
}
```

- [ ] **Step 3: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 5: C# ProxyClasses 新增属性

**Files:**
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:89-94` (添加属性)
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:184` (RefreshFromConfig)
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:209` (ApplyToConfig)

- [ ] **Step 1: 在 ConfigProxy 中添加 PenetratingModeOpacity 属性**

在 `ProxyClasses.cs:89-94` 附近，在 `private int _opacity = 80;` 后添加：
```csharp
private int _penetratingModeOpacity = 50;
public int PenetratingModeOpacity
{
    get => _penetratingModeOpacity;
    set { _penetratingModeOpacity = value; OnPropertyChanged(); }
}
```

- [ ] **Step 2: 在 RefreshFromConfig 中读取值**

在 `ProxyClasses.cs:184` 附近，在 `Opacity = config.OPACITY;` 后添加：
```csharp
PenetratingModeOpacity = config.PENETRATING_MODE_OPACITY;
```

- [ ] **Step 3: 在 ApplyToConfig 中写入值**

在 `ProxyClasses.cs:209` 附近，在 `config.OPACITY = Opacity;` 后添加：
```csharp
config.PENETRATING_MODE_OPACITY = PenetratingModeOpacity;
```

- [ ] **Step 4: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 6: ConfigWindow.xaml 新增 UI 控件

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml:432-459` (现有透明度滑块)
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml` (在现有滑块后添加新滑块)

- [ ] **Step 1: 在现有透明度滑块后添加穿透模式透明度滑块**

在 `ConfigWindow.xaml:459` 之后（`</StackPanel>` 之前），添加：
```xml
                            <StackPanel Orientation="Horizontal" Margin="8,8,0,0"
                                VerticalAlignment="Center">
                                <Label
                                    Content="穿透模式透明度"
                                    VerticalAlignment="Center"
                                    FontFamily="Segoe UI Variable"
                                    FontSize="14"
                                    Foreground="#222"
                                    Margin="0,0,8,0" />
                                <Slider
                                    Name="PenetratingModeOpacitySlider"
                                    Minimum="0"
                                    Maximum="100"
                                    Value="50"
                                    TickFrequency="1"
                                    IsSnapToTickEnabled="True"
                                    Width="150"
                                    VerticalAlignment="Center"
                                    Margin="0,0,8,0"
                                    ValueChanged="PenetratingModeOpacitySlider_ValueChanged" />
                                <TextBlock
                                    Text="{Binding ElementName=PenetratingModeOpacitySlider, Path=Value, StringFormat=F0}"
                                    VerticalAlignment="Center"
                                    FontFamily="Segoe UI Variable"
                                    FontSize="14"
                                    Foreground="#222"
                                    Width="30" />
                                </StackPanel>
```

- [ ] **Step 2: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 7: ConfigWindow.xaml.cs 绑定事件

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs:53` (初始化滑块)
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs:279` (OpacitySlider_ValueChanged)

- [ ] **Step 1: 在 FillConfig 方法中初始化穿透模式滑块**

在 `ConfigWindow.xaml.cs:53` 附近，在 `OpacitySlider.Value = config.OPACITY;` 后添加：
```csharp
PenetratingModeOpacitySlider.Value = config.PENETRATING_MODE_OPACITY;
```

- [ ] **Step 2: 添加 PenetratingModeOpacitySlider_ValueChanged 事件处理**

在 `ConfigWindow.xaml.cs:279` 附近，在 `OpacitySlider_ValueChanged` 方法后添加：
```csharp
private void PenetratingModeOpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
{
    if (!m_isInitializing && e.NewValue != oldOpacityValues["PenetratingModeOpacity"])
    {
        ToolsMain.GetConfigService().Config.PENETRATING_MODE_OPACITY = (int)e.NewValue;
        GlobalEventListener.Invoke("OpacityChanged", null);
    }
}
```

- [ ] **Step 3: 在 oldOpacityValues 中添加初始化**

在 FillConfig 中初始化：
```csharp
oldOpacityValues["PenetratingModeOpacity"] = (int)config.PENETRATING_MODE_OPACITY;
```

- [ ] **Step 4: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 8: OrderedMonsterWindow.xaml.cs 使用对应透明度

**Files:**
- Modify: `JonysandMHDanmuTools/OrderedMonsterWindow.xaml.cs:94-97`

- [ ] **Step 1: 修改 RefreshWindow 方法使用对应透明度**

将 `OrderedMonsterWindow.xaml.cs:94-97` 的代码：
```csharp
float opacity = ToolsMain.GetConfigService().Config.OPACITY / 100f;
int alphaInt = mIsLocked ? (int)(170f * opacity + 0.5f): (int)(255f * opacity + 0.5f);
```

修改为：
```csharp
float opacity = mIsLocked 
    ? ToolsMain.GetConfigService().Config.PENETRATING_MODE_OPACITY / 100f
    : ToolsMain.GetConfigService().Config.OPACITY / 100f;
int alphaInt = (int)(255f * opacity + 0.5f);
```

- [ ] **Step 2: 编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Additional Task: VEH 移除

**Files:**
- Modify: `MonsterOrderWilds/main.cpp`

**Status:** ✅ 已完成

**Description:** 在 Debug 测试过程中发现 VEH 代码导致调试异常捕获混乱。为便于调试，移除了 `main.cpp` 中的 VEH 相关代码：
- 删除 `GlobalVectoredExceptionHandler` 函数
- 删除 `RegisterGlobalVEH()` 函数  
- 删除 `wWinMain()` 中的 `RegisterGlobalVEH()` 调用

---

## Self-Review Checklist

- [x] Spec coverage: 所有 Gherkin scenarios 都有对应 task
- [x] Placeholder scan: 无 TBD/TODO
- [x] Type consistency: `penetratingModeOpacity` 字段名在 C++/C# 各层一致
- [x] 编译验证: 每步后编译确认 `0 个错误`

---

## Plan Complete

Plan saved to `openspec/changes/separate-opacity-control/tasks.md`

**Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks

**2. Inline Execution** - Execute tasks in this session

Which approach?
