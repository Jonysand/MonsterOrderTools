# 架构文档：C++ 数据处理层

## 整体架构

```
┌─────────────────────────────────────────────────┐
│                   C# UI Layer                    │
│  ConfigWindow / OrderedMonsterWindow / ToolsMain │
│  数据绑定 + 事件处理 + 用户交互                    │
├─────────────────────────────────────────────────┤
│            C++/CLI Bridge Layer (C++/CLI)        │
│  MHDanmuToolsHost / DataBridgeWrapper            │
│  TString ↔ System::String 转换                   │
├─────────────────────────────────────────────────┤
│              NativeImports (C# P/Invoke)         │
│  NativeImports.cs                                │
│  DllImport 声明 + 回调代理                       │
├─────────────────────────────────────────────────┤
│            DataBridgeExports (C++ Native)        │
│  __declspec(dllexport) 导出函数                  │
│  C# 调用入口 + C++ 回调注册                      │
├─────────────────────────────────────────────────┤
│            C++ Data Processing Layer             │
│  ConfigManager / MonsterDataManager /            │
│  PriorityQueueManager / DanmuProcessor /          │
│  StringProcessor / ErrorHandler                  │
│  JSON解析 + 数据匹配 + 优先级处理 + 播报逻辑      │
├─────────────────────────────────────────────────┤
│              C++ Foundation Layer                │
│  Network / TextToSpeech / BliveManager /         │
│  AudioPlayer / MimoTTSClient                     │
│  HTTP通信 + TTS + WebSocket + 音频播放            │
└─────────────────────────────────────────────────┘
```

## 模块职责

### C++ 数据处理层

| 模块 | 文件 | 职责 |
|------|------|------|
| ConfigManager | ConfigManager.h/cpp | 配置加载/保存/变更通知/线程安全 |
| ConfigFieldRegistry | ConfigFieldRegistry.h/cpp | C++ 字段元数据注册/变更回调 |
| MonsterDataManager | MonsterDataManager.h/cpp | 怪物JSON解析/正则匹配/图标URL |
| PriorityQueueManager | PriorityQueueManager.h/cpp | 队列入队出队/排序/持久化/定时保存 |
| DanmuProcessor | DanmuProcessor.h/cpp | 弹幕解析/点怪识别/优先级处理 |
| StringProcessor | StringProcessor.h/cpp | UTF-8转换/中文处理/名称规范化 |
| ErrorHandler | ErrorHandler.h | 统一错误记录/回调通知 |
| DataBridge | DataBridge.h | C++数据桥接接口（管理器访问点） |
| EventBridge | EventBridge.h | C++事件桥接接口 |
| DataBridgeExports | DataBridgeExports.h/cpp | __declspec(dllexport) 导出函数 |

### C# UI 层

| 模块 | 文件 | 职责 |
|------|------|------|
| ConfigWindow | ConfigWindow.xaml/cs | 配置界面UI |
| OrderedMonsterWindow | OrderedMonsterWindow.xaml/cs | 点怪队列显示窗口 |
| ToolsMain | ToolsMain.cs | 主逻辑入口/事件分发 |
| DataStructures | DataStructures.cs | 批量数据传输结构体 |
| NativeImports | NativeImports.cs | DllImport 声明 + 回调代理 |
| ProxyClasses | ProxyClasses.cs | C# 代理类（ConfigProxy, PriorityQueueProxy 等） |
| DanmuManager | DanmuManager.cs | 弹幕处理入口（路由到C++） |

## 线程模型

```
C# UI Thread (WPF Dispatcher)
    ↓ Dispatcher.InvokeAsync
C++ Main Thread (消息循环 + 100ms Timer)
    ↓ 异步回调
WebSocket Thread (接收)
    ↓ 异步
TTS Thread (COM)
```

**线程数**: 2-3 个线程

## 关键设计决策

| 决策 | 选择 |
|------|------|
| 线程模型 | 2-3线程，单线程Tick处理 |
| 任务优先级 | 静态：HIGH/NORMAL/LOW |
| TTS并发 | 串行队列，同时1个 |
| 配置应用 | 手动（点击"应用"按钮） |
| Dispatcher调用 | 全部使用InvokeAsync |
| 线程退出 | 立即退出，不等待 |
| **数据处理位置** | **全部在 C++ Native 层** |
| **C# 层职责** | **仅 UI 路由，不做业务逻辑** |

## 数据流

### 弹幕处理流程（C++/C# 分层明确）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         WebSocket 直播服务器                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  C++ Native: BliveManager                                                  │
│  - OnReceiveWSMessage (子线程接收回调)                                       │
│  - HandleWSMessage (主线程 Tick，100ms)                                     │
│  - HandleSmsReply (判断 cmd 类型)                                           │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
                          ┌───────────────────┐
                          │ cmd ==           │
                          │ "LIVE_OPEN_PLATFORM_DM" │
                          └─────────┬─────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │                               │
                    ▼                               ▼
         ┌──────────────────┐           ┌──────────────────┐
         │ TTSManager 处理   │           │ DanmuProcessor   │
         │ (语音播报)        │           │ (点怪业务)       │
         └────────┬─────────┘           └────────┬─────────┘
                  │                            │
                  │                            ▼
                  │            ┌─────────────────────────────┐
                  │            │ DanmuProcessor::ParseDanmuJson │
                  │            │ DanmuProcessor::ProcessDanmu  │
                  │            └─────────────┬───────────────┘
                  │                          │
                  │                          ▼
                  │            ┌─────────────────────────────┐
                  │            │ 处理步骤:                    │
                  │            │ ① PassesFilter (粉丝牌过滤)  │
                  │            │ ② TryUpdatePriority (优先)  │
                  │            │ ③ Contains (重复检查)       │
                  │            │ ④ GetMatchedMonsterName    │
                  │            │ ⑤ Enqueue (入队)           │
                  │            │ ⑥ NotifyDanmuProcessed     │
                  │            └─────────────┬───────────────┘
                  │                          │
                  │                          ▼
                  │            ┌─────────────────────────────┐
                  │            │ PriorityQueueManager::Enqueue│
                  │            │ - 写入 C++ 内部队列          │
                  │            └─────────────┬───────────────┘
                  │                          │
                  │                          ▼
                  │            ┌─────────────────────────────┐
                  │            │ 回调: g_danmuProcessedCallback│
                  │            │ (仅 addedToQueue==true 时)   │
                  │            └─────────────┬───────────────┘
                  │                          │
                  └──────────────────────────┼─────────────────────────────────┘
                                             │ P/Invoke 回调 (C++ → C#)
                                             │ 参数: userName, monsterName
                                             ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  C#: DanmuManager.OnDanmuProcessed (回调处理)                               │
│  - GlobalEventListener.Invoke("RefreshOrder")                              │
│  - GlobalEventListener.Invoke("AddRollingInfo", RollingInfo)                │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  C#: OrderedMonsterWindow                                                  │
│  - AddRollingInfo: 跑马灯动画                                               │
│  - RefreshOrder: 后台排序 → MainList.Items 更新                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

**关键洞察**:
1. 弹幕处理**全部在 C++ Native 层**完成，包括解析、过滤、匹配、入队
2. C# 的 DanmuManager 只负责**注册回调**和**UI 触发**，本身不做业务逻辑
3. 回调通过 `g_danmuProcessedCallback(userName, monsterName)` 触发
4. TTS 播报和点怪业务是**并行**的，都从 HandleSmsReply 出发

### 配置数据流动

#### 设计原则：C++ 是唯一的配置数据源

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           C++ 层 (单一数据源)                           │
│                                                                          │
│  DataBridge_Initialize()                                                │
│       │  调用 ConfigManager::LoadConfig() 从文件加载到 ConfigData      │
│       ▼                                                                  │
│  ConfigData config_;  ◄─── 唯一的配置数据源                            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                              │ ConfigFieldRegistry.Get/Set
┌─────────────────────────────────────────────────────────────────────────┐
│                           C# 层 (只读/写接口)                            │
│                                                                          │
│  MainConfig.XXX 属性 ──▶ ConfigFieldRegistry.Get/Set ──▶ C++          │
│       ▲                                                                   │
│       │                                                                   │
│  UI绑定 ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ← ←
│                                                                          │
│  SaveConfig() ──▶ NativeImports.Config_Save() ──▶ C++保存到文件       │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

**关键设计**：
- C++ 的 `ConfigData` 是唯一的配置数据源
- C# 的 `MainConfig` 只是一个"空壳"，通过 `ConfigFieldRegistry` 代理到 C++
- C# **不直接读写 JSON 文件**，配置文件读写全部由 C++ 处理
- 保存时：C# 调用 `Config_Save()` → C++ 的 `ConfigManager::SaveConfig()`

#### 程序启动流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│  ToolsMain.Inited() 被调用                                              │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  C# 层 - ConfigService.LoadConfig()  (先执行)                           │
│                                                                          │
│  1. _config = new MainConfig();                                        │
│       │  创建空壳对象，不读写文件                                         │
│       ▼                                                                  │
│  2. MainConfig 属性通过 ConfigFieldRegistry 访问 C++ 数据               │
│       │  此时 C++ 数据尚未加载，访问返回默认值                            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼ (同一调用链内)
┌─────────────────────────────────────────────────────────────────────────┐
│  C# 层 - MonsterData.GetInst().LoadJsonData()                          │
│       │                                                                  │
│       ▼                                                                  │
│  3. NativeImports.DataBridge_Initialize()  (后执行)                    │
│       │  P/Invoke 调用 C++                                              │
│       ▼                                                                  │
│  4. DataBridge::Initialize()                                            │
│       │                                                                  │
│       ▼                                                                  │
│  5. ConfigManager::LoadConfig()                                         │
│       │  读取 "MonsterOrderWilds_configs/MainConfig.cfg" (JSON)        │
│       ▼                                                                  │
│  6. ConfigData config_;  ◄─── C++ 配置数据加载完成                     │
│       │                                                                  │
│       ▼                                                                  │
│  7. DanmuProcessor 同步过滤条件                                          │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### 用户修改配置流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           C# 层 (UI层)                                   │
│                                                                          │
│  用户勾选 CheckBox                                                        │
│       │                                                                  │
│       ▼                                                                  │
│  CheckBox_Changed 事件                                                   │
│       │                                                                  │
│       ▼                                                                  │
│  GlobalEventListener.Invoke("ConfigChanged", "ONLY_MEDAL_ORDER:1")     │
│       │                                                                  │
│       ▼                                                                  │
│  ToolsMain.ConfigChanged("ONLY_MEDAL_ORDER:1")                          │
│       │                                                                  │
│       ▼                                                                  │
│  _Config.Config.ONLY_MEDAL_ORDER = true;   ◄─── MainConfig 属性赋值    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                              │ (P/Invoke 边界)
┌─────────────────────────────────────────────────────────────────────────┐
│                           C++ 层                                         │
│                                                                          │
│  Config_SetValue("onlyMedalOrder", "true", CONFIG_TYPE_BOOL)           │
│       │                                                                  │
│       ▼                                                                  │
│  更新 ConfigData config_;  ◄─── C++ 内存配置更新                        │
│       │                                                                  │
│       ▼                                                                  │
│  MarkDirty() + 同步到 DanmuProcessor                                    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### 保存配置流程（点击"保存设置"）

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           C# 层                                          │
│                                                                          │
│  SaveSettingsButton_Click()                                              │
│       │                                                                  │
│       ▼                                                                  │
│  ConfigService.SaveConfig()                                              │
│       │                                                                  │
│       ▼                                                                  │
│  NativeImports.Config_Save()  ──────────────▶ C++ 调用                 │
│       │                             (P/Invoke)                           │
│       │                                                                  │
│       │  注意：C# 端不再直接序列化 JSON                                  │
│       ▼                                                                  │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                           C++ 层                                         │
│                                                                          │
│  Config_Save()                                                          │
│       │                                                                  │
│       ▼                                                                  │
│  ConfigManager::SaveConfig(true)                                        │
│       │  内部序列化 ConfigData 为 JSON                                   │
│       ▼                                                                  │
│  写入 "MonsterOrderWilds_configs/MainConfig.cfg"                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### ConfigFieldRegistry 反射机制

```csharp
// MainConfig 属性不存储数据，只做跨语言转发
public bool ONLY_MEDAL_ORDER
{
    get => (bool)ConfigFieldRegistry.Get("onlyMedalOrder");  // C++ 读取
    set => ConfigFieldRegistry.Set("onlyMedalOrder", value);   // C++ 写入
}
```

**ConfigService 简化版**（C# 不再读写文件）：
```csharp
public sealed class ConfigService
{
    public MainConfig Config => _config;
    private MainConfig _config;

    public void LoadConfig()
    {
        _config = new MainConfig();  // 只创建空壳，数据从 C++ 获取
    }

    public void SaveConfig()
    {
        NativeImports.Config_Save();  // 委托给 C++ 保存
    }
}
```

#### C++/C# 配置边界总结

| 操作 | 边界方向 | 函数 |
|------|----------|------|
| **启动读取** | C++ → C# | `Config_GetBool/GetString/...` |
| **修改配置** | C# → C++ | `Config_SetValue` |
| **保存配置** | C++ 内部 | `ConfigManager::SaveConfig()` |
| **初始化顺序** | C# 先，C++ 后 | `ConfigService.LoadConfig` → `DataBridge_Initialize` |

## 核心设计原则

**所有数据处理在 C++ Native 层完成，C# 层仅保留 UI 相关逻辑。**

| 层级 | 语言 | 职责 |
|------|------|------|
| C++ Native | C++ | **数据处理**：JSON解析、业务逻辑、队列管理、排序、持久化 |
| C++/CLI | C++/CLI | **类型转换**：TString ↔ System::String |
| C# P/Invoke | C# | **路由转发**：DllImport 声明、回调代理 |
| C# UI | C# | **界面展示**：数据绑定、事件处理、用户交互 |

## API 参考

### DataBridgeExports (C++ Native → C# P/Invoke 入口)

所有 C# 调用 C++ 的入口点，使用 `__declspec(dllexport)` 导出：

```cpp
// 初始化
bool DataBridge_Initialize();                    // 初始化所有管理器
void DataBridge_Shutdown();                      // 关闭所有管理器

// 怪物数据
bool DataBridge_IsMonsterDataLoaded();           // 检查怪物数据是否加载
bool DataBridge_MatchMonsterName(inputText, outMonsterName, bufferSize, outTemperedLevel);
void DataBridge_GetMonsterIconUrl(monsterName, outUrl, bufferSize);

// 配置管理（通用 key-value 方式）
void Config_SetValue(key, value, type);          // 设置配置项（自动同步到各模块）
void Config_Save();                              // 保存配置到文件
void Config_GetString(key, outValue, bufferSize); // 获取字符串配置
void Config_GetBool(key, outValue);              // 获取布尔配置
void Config_GetInt(key, outValue);                // 获取整数配置
void Config_GetFloat(key, outValue);              // 获取浮点配置
void Config_GetDouble(key, outValue);             // 获取双精度配置

// 优先级队列
bool PriorityQueue_Contains(userId);
void PriorityQueue_Enqueue(userId, timeStamp, priority, userName, monsterName, guardLevel, temperedLevel);
void PriorityQueue_DequeueByIndex(index);
void PriorityQueue_SortQueue();
void PriorityQueue_Clear();
void PriorityQueue_SaveList();
bool PriorityQueue_LoadList();
void PriorityQueue_GetAllNodes(callback, userData);  // 获取所有队列节点（回调方式）

// 弹幕处理
void DanmuProcessor_ProcessDanmu(jsonStr);      // 处理单条弹幕

// 回调注册
void DataBridge_SetDanmuProcessedCallback(callback, userData);  // 注册弹幕处理完成回调
```

### NativeImports (C# P/Invoke 声明)

```csharp
internal static class NativeImports
{
    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void DanmuProcessor_ProcessDanmu(string jsonStr);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern bool PriorityQueue_Contains(string userId);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_SetValue(string key, string value, int type);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_Save();

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_GetString(string key, System.Text.StringBuilder outValue, int bufferSize);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_GetBool(string key, out bool outValue);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_GetInt(string key, out int outValue);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_GetFloat(string key, out float outValue);

    [DllImport("MonsterOrderWilds.exe", CallingConvention = CallingConvention.StdCall)]
    public static extern void Config_GetDouble(string key, out double outValue);

    // ... 其他 DllImport 声明

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void DanmuProcessedCallback(string userName, string monsterName, IntPtr userData);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void OnQueueNodeCallback(string userId, long timeStamp, bool priority, string userName, string monsterName, int guardLevel, int temperedLevel, IntPtr userData);

    public enum ConfigFieldType
    {
        String = 0,
        Bool = 1,
        Int = 2,
        Float = 3,
        Double = 4
    }
}
```

### ConfigManager
```cpp
bool LoadConfig();                           // 加载配置
bool SaveConfig(bool force = false);         // 保存配置
void MarkDirty();                            // 标记已修改
const ConfigData& GetConfig() const;         // 获取配置（线程安全）
void UpdateConfig(const ConfigData& data);   // 批量更新配置
void Set*();                                 // 单字段设置（线程安全）
void AddConfigChangedListener();             // 注册变更回调
```

### Config_SetValue 字段类型

| 字段名 | 类型 | 说明 | 必要性 |
|--------|------|------|--------|
| idCode | String | 身份码 | ✅ |
| onlyMedalOrder | Bool | 仅粉丝牌可点怪 | ✅ |
| enableVoice | Bool | 启用语音播报 | ✅ |
| speechRate | Int | 语速 | ✅ |
| speechPitch | Int | 语调 | ✅ |
| speechVolume | Int | 音量 | ✅ |
| onlySpeekWearingMedal | Bool | 仅播报佩戴粉丝牌 | ✅ |
| onlySpeekGuardLevel | Int | 仅播报舰长等级 | ✅ |
| onlySpeekPaidGift | Bool | 仅播报付费礼物 | ✅ |
| opacity | Int | 窗口透明度 | ✅ |
| ttsEngine | String | TTS引擎 | ✅ |
| mimoApiKey | String | MiMo API密钥 | ✅ |
| mimoVoice | String | MiMo语音 | ✅ |
| mimoStyle | String | MiMo风格 | ✅ |
| mimoSpeed | Float | MiMo语速 | ✅ |
| topPosX | Double | 窗口X坐标 | ✅ |
| topPosY | Double | 窗口Y坐标 | ✅ |

**自动同步**：设置 `onlyMedalOrder`、`onlySpeekWearingMedal`、`onlySpeekGuardLevel`、`onlySpeekPaidGift` 时，通过 `ConfigFieldRegistry` 的 `onChanged` 回调自动同步到 DanmuProcessor。

### C++ ConfigFieldRegistry (避免硬编码)

**设计目的**：通过成员指针 + offsetof 机制避免字段名硬编码，配合变更回调实现配置同步。

**核心结构**：

```cpp
// 字段类型
enum class ConfigFieldType : int {
    String = 0, Bool = 1, Int = 2, Float = 3, Double = 4
};

// 字段元数据（包含偏移量和可选回调）
struct ConfigFieldMeta {
    const char* name;           // 字段名
    ConfigFieldType type;       // 字段类型
    size_t offset;             // 在 ConfigData 中的偏移量
    ConfigChangeHandler onChanged;  // 变更回调（可选）
};

// 字段注册宏
#define REGISTER_FIELD(name, type, member, fieldType) \
    GetMutableFields().emplace_back(name, fieldType, offsetof(ConfigData, member))

#define REGISTER_FIELD_WITH_CALLBACK(name, type, member, fieldType, callback) \
    GetMutableFields().emplace_back(name, fieldType, offsetof(ConfigData, member), callback)
```

**字段注册示例**：

```cpp
void ConfigFieldRegistry::RegisterAll() {
    // 普通字段：无回调
    REGISTER_FIELD("idCode", std::string, idCode, ConfigFieldType::String);
    REGISTER_FIELD("opacity", int, opacity, ConfigFieldType::Int);

    // 特殊字段：带回调（自动同步到 DanmuProcessor）
    REGISTER_FIELD_WITH_CALLBACK("onlyMedalOrder", bool, onlyMedalOrder, ConfigFieldType::Bool,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlyMedalOrder(cfg.onlyMedalOrder);
        });

    REGISTER_FIELD_WITH_CALLBACK("onlySpeekGuardLevel", int, onlySpeekGuardLevel, ConfigFieldType::Int,
        [](ConfigData& cfg) {
            DanmuProcessor::Inst()->SetOnlySpeekGuardLevel(cfg.onlySpeekGuardLevel);
        });
}
```

**ConfigFieldRegistry 核心方法**：

```cpp
class ConfigFieldRegistry {
public:
    static void RegisterAll();
    static const ConfigFieldMeta* Find(const char* name);
    static const ConfigFieldMeta* FindByOffset(size_t offset);

    // 类型安全访问
    static const char* GetString(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetString(const ConfigFieldMeta* meta, ConfigData& config, const char* value);
    static bool GetBool(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetBool(const ConfigFieldMeta* meta, ConfigData& config, bool value);
    static int GetInt(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetInt(const ConfigFieldMeta* meta, ConfigData& config, int value);
    static float GetFloat(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetFloat(const ConfigFieldMeta* meta, ConfigData& config, float value);
    static double GetDouble(const ConfigFieldMeta* meta, const ConfigData& config);
    static void SetDouble(const ConfigFieldMeta* meta, ConfigData& config, double value);

    // 触发变更回调
    static void InvokeOnChanged(const ConfigFieldMeta* meta, ConfigData& config);
};
```

**Config_SetValue 通用实现**（无硬编码）：

```cpp
void Config_SetValue(const char* key, const char* value, int type) {
    auto& config = const_cast<ConfigData&>(ConfigManager::Inst()->GetConfig());
    auto* meta = ConfigFieldRegistry::Find(key);

    switch (type) {
    case CONFIG_TYPE_STRING: ConfigFieldRegistry::SetString(meta, config, value); break;
    case CONFIG_TYPE_BOOL:   ConfigFieldRegistry::SetBool(meta, config, strcmp(value,"true")==0); break;
    case CONFIG_TYPE_INT:    ConfigFieldRegistry::SetInt(meta, config, atoi(value)); break;
    case CONFIG_TYPE_FLOAT:  ConfigFieldRegistry::SetFloat(meta, config, atof(value)); break;
    case CONFIG_TYPE_DOUBLE: ConfigFieldRegistry::SetDouble(meta, config, atof(value)); break;
    }

    ConfigManager::Inst()->MarkDirty();
    ConfigFieldRegistry::InvokeOnChanged(meta, config);
}
```

**新增字段同步逻辑的步骤**：

1. 在 `ConfigData` 结构体中添加成员
2. 在 `ConfigFieldRegistry::RegisterAll()` 中添加一行

**无需修改** `Config_SetValue` 或 `Config_Get*` 函数。

### ConfigFieldRegistry (C# 反射机制)

**设计目的**：通过字典实现 C++ 配置数据和 C# 层数据的自动同步，便于后续扩展。

**核心结构**：
```csharp
// 字段元数据
public class ConfigFieldInfo {
    public string Name { get; set; }      // C++ 字段名
    public ConfigFieldType Type { get; set; }
    public Func<object> Getter { get; set; }  // C# 侧 getter
    public Action<object> Setter { get; set; } // C# 侧 setter
}

// 配置字段注册表
public static class ConfigFieldRegistry {
    private static readonly Dictionary<string, ConfigFieldInfo> _fields;

    static ConfigFieldRegistry() {
        // 注册所有字段
        Register("idCode", ConfigFieldType.String,
            () => GetString("idCode"),
            v => SetValue("idCode", (string)v, ConfigFieldType.String));
        // ... 其他字段
    }

    public static object Get(string name);
    public static void Set(string name, object value);
}
```

**优点**：
- **新增字段只需一处**：在 ConfigFieldRegistry 静态构造函数添加一行
- **类型安全**：每种类型有专门的 getter/setter
- **自动同步**：设置值时自动调用 NativeImports
- **可扩展**：支持新增更多类型

**使用示例**：
```csharp
// 获取配置值
bool onlyMedal = (bool)ConfigFieldRegistry.Get("onlyMedalOrder");

// 设置配置值
ConfigFieldRegistry.Set("opacity", 80);

// MainConfig 属性使用反射
[JsonProperty]
public bool ONLY_MEDAL_ORDER {
    get => (bool)ConfigFieldRegistry.Get("onlyMedalOrder");
    set { ConfigFieldRegistry.Set("onlyMedalOrder", value); OnPropertyChanged(); }
}
```

### PriorityQueueManager
```cpp
void Enqueue(const QueueNodeData& node);     // 入队
QueueNodeData Dequeue(int index);            // 出队
void SortQueue();                            // 排序
void Clear();                                // 清空
int GetCount() const;                        // 获取队列大小
std::vector<QueueNodeData> GetAllNodes();    // 批量获取（线程安全）
bool LoadList();                             // 加载队列
bool SaveList();                             // 保存队列
void Tick();                                 // 定时保存
```

### DanmuProcessor
```cpp
DanmuProcessResult ProcessDanmu(const DanmuData& danmu);              // 处理单条弹幕
std::vector<DanmuProcessResult> ProcessDanmuBatch(...);               // 批量处理
DanmuData ParseDanmuJson(const std::string& jsonStr) const;           // JSON解析（支持完整格式和data格式）
std::string GenerateSpeakText(const std::string& user, const std::string& monster);  // 播报文本

// DanmuProcessResult 结构
struct DanmuProcessResult {
    bool matched = false;           // 是否匹配到怪物
    std::string userName;         // 用户名称
    std::string monsterName;      // 匹配的怪物名称
    int temperedLevel = 0;        // 历战等级
    bool addedToQueue = false;    // 是否加入队列
    bool shouldSpeak = false;     // 是否需要语音播报
    std::string speakText;        // 播报文本
    bool priorityUpdated = false; // 是否更新了优先状态
};
```

### DataBridge
```cpp
static ConfigData GetAllConfig();                    // 批量获取配置
static std::vector<QueueNodeData> GetAllQueueNodes(); // 批量获取队列
static void UpdateAllConfig(const ConfigData& data); // 批量更新配置
static bool Initialize();                            // 初始化所有管理器
static void Shutdown();                              // 关闭所有管理器
static void RegisterErrorCallback();                 // 注册错误回调
```

## 配置文件格式

### MainConfig.cfg (JSON)
```json
{
    "ID_CODE": "身份码",
    "ONLY_MEDAL_ORDER": true,
    "ENABLE_VOICE": false,
    "SPEECH_RATE": 0,
    "SPEECH_PITCH": 0,
    "SPEECH_VOLUME": 80,
    "ONLY_SPEEK_WEARING_MEDAL": false,
    "ONLY_SPEEK_GUARD_LEVEL": 0,
    "ONLY_SPEEK_PAID_GIFT": false,
    "OPACITY": 100,
    "TTS_ENGINE": "auto",
    "MIMO_VOICE": "mimo_default",
    "MIMO_STYLE": "",
    "MIMO_SPEED": 1.0,
    "MIMO_API_KEY": "",
    "TopPos": {"X": 0, "Y": 0}
}
```

### OrderList.list (JSON Array)
```json
[
    {
        "UserId": "用户ID",
        "TimeStamp": 1700000000,
        "Priority": true,
        "UserName": "用户名",
        "MonsterName": "怪物名",
        "GuardLevel": 3,
        "TemperedLevel": 0
    }
]
```

## 问题修复记录

### 2026-03-30: UTF-8/Unicode 编码问题导致弹幕处理失败

**问题描述**：
点怪功能失效，弹幕消息中的中文无法被正则表达式匹配。日志显示：
- `std::regex` 无法匹配 UTF-8 中文字符
- C++ 传递回 C# 的中文字符串显示为乱码

**根本原因**：
1. **C++ `std::regex` 不支持 UTF-8**: MSVC 的 `std::regex` 默认按单字节（ASCII）处理，无法正确识别 UTF-8 编码的中文字符
2. **P/Invoke 字符集不匹配**: C++ 导出函数使用 UTF-8 编码传递字符串，但 C# `CharSet.Ansi` 以为是 GBK/ASCII

**修复方案**：

#### 1. C++ 端使用 `std::wregex`（宽字符正则）

将所有正则表达式改为宽字符版本：

```cpp
// 旧代码（UTF-8 不支持）
orderPatterns_.push_back({ std::regex("^点怪"), "点怪" });
std::smatch match;
std::regex_search(normalizedMsg, match, pattern);

// 新代码（宽字符支持）
orderPatterns_.push_back({ std::wregex(L"^点怪"), L"点怪" });
std::wsmatch match;
std::regex_search(wnormalizedMsg, match, pattern);
```

添加 UTF-8 ↔ UTF-16 转换函数：

```cpp
static std::wstring Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

static std::string WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}
```

#### 2. P/Invoke 回调使用 `CharSet.Unicode`

C++ 回调函数签名改为宽字符：

```cpp
// DataBridgeExports.h
typedef void(__stdcall* OnDanmuProcessedCallback)(const wchar_t* userName, const wchar_t* monsterName, void* userData);
typedef void(__stdcall* OnQueueNodeCallback)(const wchar_t* userId, long long timeStamp, bool priority, const wchar_t* userName, const wchar_t* monsterName, int guardLevel, int temperedLevel, void* userData);
```

C++ 回调实现时转换编码：

```cpp
DataBridge::GetDanmuProcessor()->AddDanmuProcessedListener([](const DanmuProcessResult& result) {
    if (g_danmuProcessedCallback != nullptr && result.addedToQueue)
    {
        std::wstring wuserName = Utf8ToWstring(result.userName);
        std::wstring wmonsterName = Utf8ToWstring(result.monsterName);
        g_danmuProcessedCallback(wuserName.c_str(), wmonsterName.c_str(), g_danmuProcessedUserData);
    }
});
```

C# 回调代理使用 Unicode：

```csharp
// NativeImports.cs
[UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
public delegate void DanmuProcessedCallback(string userName, string monsterName, IntPtr userData);

[UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
public delegate void OnQueueNodeCallback(string userId, long timeStamp, bool priority, string userName, string monsterName, int guardLevel, int temperedLevel, IntPtr userData);
```

#### 3. MSVC 编译选项 `/utf-8`

在 `MonsterOrderWilds.vcxproj` 中添加：

```xml
<ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
  <ClCompile>
    <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>
  </ClCompile>
</ItemDefinitionGroup>
```

**修改的文件**：
- `MonsterOrderWilds/DanmuProcessor.cpp` - 使用 `std::wregex` 和 UTF-8 ↔ UTF-16 转换
- `MonsterOrderWilds/DanmuProcessor.h` - 更改正则表达式类型为宽字符
- `MonsterOrderWilds/MonsterDataManager.cpp` - 使用 `std::wregex` 和 UTF-8 ↔ UTF-16 转换
- `MonsterOrderWilds/MonsterDataManager.h` - 更改正则表达式类型为宽字符
- `MonsterOrderWilds/DataBridgeExports.h` - 回调函数签名改为 `wchar_t*`
- `MonsterOrderWilds/DataBridgeExports.cpp` - 回调中转换编码为 UTF-16
- `JonysandMHDanmuTools/NativeImports.cs` - 回调代理改为 `CharSet.Unicode`
- `MonsterOrderWilds/MonsterOrderWilds.vcxproj` - 添加 `/utf-8` 编译选项

**经验教训**：
- Windows 平台 C++ `std::regex` 不支持 UTF-8，必须使用 `std::wregex`
- P/Invoke 跨语言传递中文字符串时，必须使用 `CharSet.Unicode`（UTF-16）
- 源码文件应保存为 UTF-8 编码，并配置编译器 `/utf-8` 选项
- 中文字符串正则匹配不能使用 `\b`（word boundary），因为中文不是"单词"

### 2026-03-30: 日志输出中文乱码修复

**问题描述**：
日志文件中中文显示为乱码，例如 `UserName=涓×�` 而非 `UserName=用户名`。

**根本原因**：
1. `TEXT("%s")` 在 UNICODE 模式下不会自动变成 `%ls`，导致 `fwprintf` 把宽字符串当作窄字符串解析
2. `LOG_DEBUG(TEXT("...%hs..."))` 中的 `%hs` 在宽格式字符串中将 UTF-8 字符串按系统代码页（GBK）解释，而非 UTF-8

**修复方案**：

#### 1. WriteLog.cpp 格式符修正

```cpp
// 旧代码
_ftprintf(g_fp, TEXT("%s %s\n"), strLogTag, strLogContext);

// 新代码
_ftprintf(g_fp, TEXT("%s %ls\n"), strLogTag, strLogContext);
```

#### 2. WriteLog.h 添加宽字符串日志宏

```cpp
static inline std::wstring Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

static inline std::string WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}

#define LOGW_DEBUG(EXPR, ...) \
    { wchar_t __buf__[4096]; _snwprintf_s(__buf__, 4096, _TRUNCATE, EXPR, __VA_ARGS__); WriteLog::WriteLog(WriteLog::LogLevel_DEBUG, __buf__); }
```

#### 3. 调用点使用宽字符串宏

```cpp
// 旧代码（乱码）
LOG_DEBUG(TEXT("[DanmuProcessor] user=%hs, msg=%hs"), danmu.userName.c_str(), danmu.message.c_str());

// 新代码（正常显示）
std::wstring wuserName = Utf8ToWstring(danmu.userName);
std::wstring wmessage = Utf8ToWstring(danmu.message);
LOGW_DEBUG(L"[DanmuProcessor] user=%s, msg=%s", wuserName.c_str(), wmessage.c_str());
```

**修改的文件**：
- `MonsterOrderWilds/WriteLog.h` - 添加 `LOGW_DEBUG/INFO/WARNING/ERROR` 宏和 `Utf8ToWstring/WstringToUtf8` 函数
- `MonsterOrderWilds/WriteLog.cpp` - `%s` → `%ls`
- `MonsterOrderWilds/DanmuProcessor.cpp` - `LOG_DEBUG` → `LOGW_DEBUG`，使用宽字符串
- `MonsterOrderWilds/DataBridgeExports.cpp` - `LOG_DEBUG` → `LOGW_DEBUG`，使用宽字符串
- `MonsterOrderWilds/MonsterDataManager.cpp` - `LOG_DEBUG` → `LOGW_DEBUG`，使用宽字符串

**经验教训**：
- Windows 宽字符格式化中，`TEXT("%s")` 不会自动转为 `%ls`，必须显式使用 `%ls`
- `%hs` 在宽格式字符串中按系统代码页（GBK）转换 UTF-8 字节，导致中文乱码
- UTF-8 中文字符串必须先转为 UTF-16 宽字符串，再用 `%s` 输出

### 2026-03-30: 点怪图标不显示修复

**问题描述**：
点怪后列表显示怪物名称，但图标不显示。

**根本原因**：
`DataBridge_GetMonsterIconUrl` 使用 `CharSet.Ansi`，但怪物名称是中文 UTF-8。名称传递时编码不匹配，导致 `GetMatchedMonsterIconUrl` 找不到匹配的怪物，返回空字符串。

**修复方案**：
1. C++ 端函数签名改为宽字符
2. C# 端 P/Invoke 改为 `CharSet.Unicode`
3. C++ 内部做 UTF-16 ↔ UTF-8 转换

**修改的文件**：
- `MonsterOrderWilds/DataBridgeExports.h` - `DataBridge_GetMonsterIconUrl` 签名改为 `wchar_t*`
- `MonsterOrderWilds/DataBridgeExports.cpp` - 内部转换编码
- `JonysandMHDanmuTools/NativeImports.cs` - `CharSet.Ansi` → `CharSet.Unicode`

**经验教训**：
- 所有涉及中文字符串的 P/Invoke 函数必须使用 `CharSet.Unicode`
- 不能假设 `CharSet.Ansi` 能正确处理 UTF-8 中文字符串

### 2026-03-30: 剩余 P/Invoke 函数编码修复

**问题描述**：
`DataBridge_MatchMonsterName` 和 `PriorityQueue_Enqueue` 仍使用 `CharSet.Ansi`，可能导致中文怪物名称和用户名处理异常。

**修复方案**：
1. `DataBridge_MatchMonsterName` - 改为宽字符接口
2. `PriorityQueue_Enqueue` - userName 和 monsterName 改为宽字符

**修改的文件**：
- `MonsterOrderWilds/DataBridgeExports.h` - 签名改为 `wchar_t*`
- `MonsterOrderWilds/DataBridgeExports.cpp` - 内部转换编码
- `JonysandMHDanmuTools/NativeImports.cs` - `CharSet.Unicode`

## 9. 单元测试

### 测试基础设施

由于项目使用 `/clr`（WPF/CLR混合），`std::cout` 输出在测试中不可用。解决方案：

1. **UnitTestLog.h** - 定义 `TestLog(const char* msg)` 接口
2. **UnitTestMain.cpp** - 实现 `TestLog`，使用 Windows API（`CreateFileA`/`WriteFile`）写文件
3. **测试文件** - 使用 `TestLog` 替代 `std::cout`
4. **RunTests.bat** - 方便的测试运行脚本

### 测试结果 (43/43 PASS)

```
========================================
  MonsterOrderWilds Unit Tests
========================================

ConfigManager Tests:        8 PASS
MonsterDataManager Tests:  10 PASS  
PriorityQueueManager Tests: 6 PASS
DanmuProcessor Tests:       4 PASS
StringProcessor Tests:     10 PASS
DataBridge Tests:           5 PASS

========================================
  All tests completed! (43/43 PASS)
========================================
```

测试结果文件：`x64\Debug\test_results.txt`

### 测试覆盖分析

| 模块 | 测试数 | 覆盖功能 |
|------|--------|----------|
| ConfigManager | 8 | 默认值、设置各项配置、保存加载、事件通知 |
| MonsterDataManager | 10 | JSON加载、名称匹配、别名匹配、历战等级、图标URL |
| PriorityQueueManager | 6 | 入队、出队、Contains、优先级排序、清空 |
| DanmuProcessor | 4 | JSON解析、播报文本生成、粉丝牌过滤 |
| StringProcessor | 10 | UTF-8/宽字符转换、名称规范化、中文检测、Trim/Split/Replace |
| DataBridge | 5 | 管理器获取、配置获取、队列获取、事件回调 |

### 未覆盖的功能（需要外部依赖）

| 模块 | 障碍类型 |
|------|----------|
| Network | WinHTTP API - 需要真实HTTP请求 |
| BliveManager (网络部分) | WebSocket - 需要B站服务器连接 |
| MimoTTSClient::RequestTTS | HTTP API - 需要小米服务器 |
| TextToSpeech | Windows SAPI COM组件 |
| AudioPlayer | Windows MCI多媒体API |

### 10. 经验教训总结

#### 编码相关

- UTF-8 中文字符串在 WPF/CLR 项目中处理需特别注意
- `std::cout` 在 `/clr` 下不可用，需使用 Windows API 或日志系统
- P/Invoke 函数必须使用 `CharSet.Unicode` 处理中文字符串

#### 测试相关

- 测试入口点需要使用 Windows API 写文件，不能依赖 `std::cout`
- `/clr` 项目中 CRT 初始化顺序可能导致 stdout 重定向失败
- 使用 `CreateFileA`/`WriteFile` 是可靠的测试日志方案
