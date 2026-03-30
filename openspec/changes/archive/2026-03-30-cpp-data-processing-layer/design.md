## Context

当前项目是一个C++/C#混合应用，存在职责划分不清的问题：
- **C#层**：同时承担UI交互（WPF窗口、控件）和数据处理（JSON解析、业务逻辑）
- **C++层**：主要处理网络通信（WebSocket、HTTPS）和TTS语音合成

这种混合职责架构导致：
1. **架构耦合**：UI逻辑与业务逻辑混合，难以维护和测试
2. **性能瓶颈**：数据在C#和C++层之间多次传递，增加序列化开销
3. **开发效率低**：相同的数据处理逻辑在两个层中重复实现
4. **扩展困难**：新增功能需要在两个层中同步修改

**已实现的解决方案**：
通过 P/Invoke 建立清晰的跨层调用路径：
- C++ 层使用 `__declspec(dllexport)` 导出函数
- C# 层使用 `DllImport` 声明导入
- C++ 通过回调函数通知 C# 层

**核心洞察**：弹幕数据处理流程中，DanmuManager 充当**纯路由角色**：
```
C++ Native (BliveManager) 
    → C++/CLI (MHDanmuToolsHost)
    → C# (ToolsMain → DanmuManager)
    → P/Invoke (NativeImports)
    → C++ Native (DanmuProcessor) ← 核心处理在这里
    → 回调 → C# (DanmuManager.OnDanmuProcessed)
```

项目约束：
- 必须保持现有功能不变
- 需要支持UTF-8编码（包括BOM）
- 需要兼容现有的JSON数据格式
- C++项目使用nlohmann/json（header-only库）
- 需要保持WPF数据绑定机制

## Goals / Non-Goals

**Goals:**
- 实现清晰的职责分离：C#层仅UI交互，C++层所有数据处理
- 提高数据处理性能，减少跨层调用和序列化开销
- 简化架构，提高代码可维护性和可测试性
- 保持现有用户体验和功能完整性
- 为未来功能扩展提供清晰的架构基础
- **所有数据处理在C++ Native层完成，C#层仅保留UI相关逻辑**

**Non-Goals:**
- 不改变现有的UI外观和用户体验
- 不修改JSON数据文件的格式
- 不引入新的第三方依赖（继续使用nlohmann/json）
- 不重构现有的网络通信协议
- 不改变WPF数据绑定机制

## Decisions

### 1. 采用五层架构设计
**选择**: 
```
C# UI层 → C++/CLI桥接层 → NativeImports(P/Invoke) → DataBridgeExports(dllexport) → C++数据处理层
```

**理由**:
- 清晰的职责分离，每层专注特定功能
- C#层专注于UI渲染和用户交互
- C++/CLI层仅做类型转换（TString ↔ System::String）
- P/Invoke层声明导出函数和回调代理
- C++ Native层专注于数据处理和业务逻辑

**架构图**:
```
┌─────────────────────────────────────────────────────────┐
│                    C# UI层 (瘦客户端)                    │
├─────────────────────────────────────────────────────────┤
│  UI组件层         │  数据绑定层        │  事件响应层    │
│  - Window/Page    │  - ViewModel       │  - Command     │
│  - UserControl    │  - Observable集合  │  - Event       │
│  - 数据模板       │  - 转换器          │  - 动画        │
└─────────────────────────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────────────────────────┐
│            C++/CLI 桥接层 (类型转换)                    │
│  MHDanmuToolsHost                                       │
│  TString ↔ System::String                               │
└─────────────────────────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────────────────────────┐
│            NativeImports (C# P/Invoke)                  │
│  NativeImports.cs                                       │
│  DllImport 声明 + 回调代理                              │
└─────────────────────────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────────────────────────┐
│            DataBridgeExports (C++ Native)                │
│  __declspec(dllexport) 导出函数                         │
│  C# 调用入口 + C++ 回调注册                            │
└─────────────────────────────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────────────────────────┐
│                    C++ 数据处理层                        │
├─────────────────────────────────────────────────────────┤
│  数据管理层       │  业务逻辑层        │  网络通信层    │
│  - ConfigManager  │  - DanmuProcessor  │  - BliveManager│
│  - MonsterManager │  - PriorityQueue   │  - HttpClient  │
│  - StringProcessor│  - MatchAlgorithm  │  - TTSManager  │
└─────────────────────────────────────────────────────────┘
```

### 2. 数据代理模式
**选择**: C#层通过代理类访问C++数据，而不是直接调用C++方法。

**理由**:
- 保持C#层的简洁性，只需了解代理接口
- 隐藏C++实现细节，提供稳定的API
- 便于实现数据变更通知和UI更新

**示例设计**:
```csharp
// C#层代理类
public class ConfigProxy {
    private readonly IntPtr _nativeHandle;
    
    public string UserName {
        get => NativeGetUserName(_nativeHandle);
        set => NativeSetUserName(_nativeHandle, value);
    }
    
    public event PropertyChangedEventHandler PropertyChanged;
    
    // P/Invoke或C++/CLI调用
    [DllImport("CppDataProcessing.dll")]
    private static extern string NativeGetUserName(IntPtr handle);
}
```

### 3. 事件驱动架构
**选择**: C++层通过事件通知C#层更新UI，而不是C#层轮询C++层。

**理由**:
- 减少不必要的跨层调用
- 实时响应数据变化
- 提高系统响应性

**事件类型**:
- 配置变更事件
- 队列更新事件
- 弹幕处理完成事件
- 错误发生事件

### 4. 线程模型设计
**选择**: C++层在后台线程处理数据，通过Dispatcher通知UI线程更新。

**理由**:
- 避免阻塞UI线程，保持界面响应性
- 充分利用多核CPU进行数据处理
- 符合WPF的线程单元模型

**线程架构**:
```
UI线程 (WPF Dispatcher)
├── 数据绑定更新
├── 用户输入处理
└── 动画渲染

C++工作线程池
├── 数据处理任务
├── JSON解析
├── 业务逻辑计算
└── 文件I/O操作

C++定时线程
├── 定时保存
├── 状态检查
└── 缓存清理
```

### 5. 内存管理策略
**选择**: 使用智能指针和RAII模式管理C++内存，通过C++/CLI安全传递给C#。

**理由**:
- 避免内存泄漏
- 简化内存管理
- 提供异常安全

**实现**:
```cpp
// 使用shared_ptr管理对象生命周期
class DataManager {
private:
    std::shared_ptr<ConfigManager> config_manager_;
    std::shared_ptr<MonsterDataManager> monster_manager_;
    
public:
    // 返回托管指针给C++/CLI
    ConfigManager* GetConfigManager() { return config_manager_.get(); }
};
```

### 6. 线程模型设计（低线程数）
**选择**: 单线程处理 + 异步I/O，最小化线程数量

**约束**: 整个程序使用的线程数尽量低

**线程架构** (2-3个线程):
```
C# UI线程 (WPF Dispatcher) - 必需
├── 纯UI交互和显示
├── 通过代理类访问C++数据
└── 接收C++事件通知更新UI

C++主线程 (消息循环) - 必需，承担所有数据处理
├── 定时器触发 Tick()
├── 所有数据处理（JSON解析、队列管理、怪物匹配）
├── 异步I/O回调处理
├── HTTP请求（异步协程）
└── TTS处理（异步请求）

WebSocket接收线程 - 可选（保持现有实现）
└── 接收WebSocket消息，回调通知主线程
```

**任务调度** (单线程):
```cpp
// 在主线程Tick中处理任务队列
void Tick() {
    // 1. 处理异步I/O完成事件
    // 2. 处理高优先级任务（弹幕处理、配置保存）
    // 3. 处理普通优先级任务（数据匹配、字符串处理）
    // 4. 处理低优先级任务（定时保存、日志）
}
```

**静态优先级映射**:
| 任务类型 | 优先级 | 示例 |
|----------|--------|------|
| 弹幕点怪处理 | HIGH | `DanmuProcessor::ProcessDanmu()` |
| 配置保存 | HIGH | `ConfigManager::SaveConfig()` |
| 队列操作 | HIGH | `PriorityQueueManager::Enqueue()` |
| 配置加载 | NORMAL | `ConfigManager::LoadConfig()` |
| 怪物数据匹配 | NORMAL | `MonsterDataManager::MatchMonster()` |
| 字符串处理 | NORMAL | `StringProcessor::Normalize()` |
| 定时保存 | LOW | 定时器触发的批量保存 |
| 日志写入 | LOW | 异步日志记录 |

**线程安全策略** (简化):
- 由于主要在单线程处理，大部分数据无需锁保护
- WebSocket接收线程与主线程之间使用消息队列 + 互斥锁
- 原子变量用于状态标志

### 7. 内存安全优先策略
**选择**：优先修复已知内存泄漏，使用 RAII 和智能指针避免新泄漏

**理由**：
- 内存泄漏导致长期运行时性能下降
- 修复成本低（仅修改一个函数和4个调用点）
- 使用 TString 避免手动内存管理错误

**备选方案**：
- **保持原样**：风险高，泄漏累积
- **引入复杂监控**：过度工程，成本高

### 7. 轻量级测试策略
**选择**：使用现有 #ifdef RUN_UNIT_TESTS 方案，不引入 Google Test

**理由**：
- 无额外依赖，保持项目轻量
- 与现有测试代码风格一致
- 满足当前测试需求

**备选方案**：
- **Google Test**：功能丰富但增加依赖
- **Catch2**：轻量但需迁移现有测试

## 内存管理策略

### 已知问题修复
- **ConvertToTCHAR 内存泄漏**：修改函数返回 TString，消除手动内存管理
- **修复范围**：Network.cpp 第30-49行，4个调用点（第200、282、287、352行）

### 轻量级内存监控
- **目标**：修复已知泄漏，不引入复杂监控系统
- **方法**：使用 AddressSanitizer 进行验证
- **工具**：Visual Studio 诊断工具（调试时使用）

## 单元测试策略

### 测试框架选择
- **选择**：保持现有方案（#ifdef RUN_UNIT_TESTS）
- **理由**：轻量、无额外依赖、与现有代码风格一致

### 测试覆盖目标
- **核心数据管理器**：80% 覆盖率
- **C++/CLI 互操作层**：90% 覆盖率
- **关键业务逻辑**：100% 覆盖率

### 测试文件组织
```
MonsterOrderWilds/
├── ConfigManagerTests.cpp
├── MonsterDataManagerTests.cpp
├── PriorityQueueManagerTests.cpp
├── DanmuProcessorTests.cpp
└── StringProcessorTests.cpp
```

## Risks / Trade-offs

### 风险
1. **架构复杂性增加** → 通过详细的架构文档和代码规范缓解
2. **调试困难** → 增加详细的日志记录和错误追踪
3. **性能开销** → 通过批量处理和缓存优化
4. **学习曲线** → 提供培训和代码示例
5. **集成风险** → 采用渐进式迁移，每个阶段都有验证

### 权衡
1. **开发时间 vs 维护性**: 增加前期开发时间，但长期维护成本降低
2. **性能 vs 灵活性**: 统一处理提高性能，但某些场景灵活性降低
3. **复杂性 vs 可扩展性**: 架构复杂性增加，但未来扩展更容易
4. **内存使用 vs 性能**: C++内存管理更复杂，但性能更好

### 迁移策略
**渐进式迁移**:
1. **阶段1**: 建立C++数据处理基础框架
2. **阶段2**: 迁移核心数据处理模块
3. **阶段3**: 迁移业务逻辑
4. **阶段4**: 优化和集成

**向后兼容**:
- 保持现有数据格式兼容
- 保持现有API接口兼容
- 提供迁移工具和文档

**风险控制**:
- 每个阶段都有回滚方案
- 详细的测试覆盖
- 性能监控和基准测试

### 回滚策略
- 保留C#层的数据处理代码作为备份
- 使用特性开关控制新旧实现的切换
- 分模块迁移，降低风险
- 详细的回滚文档和步骤

## TTS异步化设计

### 当前阻塞点分析
| 位置 | 阻塞类型 | 阻塞时间 |
|------|----------|----------|
| `SpeakWithMimo()` 第340-344行 | 同步等待HTTP响应 | 最多5秒 |
| `ExecuteWithRetry()` 第253-258行 | 同步等待协程 | 可变 |
| `WaitForCompletion()` 第103-141行 | 同步等待播放完成 | 最多10秒 |

### 异步化方案：状态机 + 回调

**设计原则**:
- 不使用额外线程（符合低线程数约束）
- 使用状态机管理TTS请求生命周期
- 在主线程Tick中处理状态转换
- 使用回调处理异步完成事件

**状态机设计**:
```cpp
enum class AsyncTTSState {
    Pending,        // 等待处理
    Requesting,     // 正在请求API
    Playing,        // 正在播放音频
    Completed,      // 完成
    Failed          // 失败
};

struct AsyncTTSRequest {
    TString text;
    AsyncTTSState state = AsyncTTSState::Pending;
    std::string audioData;
    std::string responseFormat;
    std::chrono::steady_clock::time_point startTime;
    int retryCount = 0;
    std::string errorMessage;
};
```

**流程对比**:
```
当前流程（同步阻塞）：
Speak() → SpeakWithMimo() → [等待5秒] → [播放] → [等待10秒] → 完成

目标流程（异步非阻塞）：
Speak() → 创建AsyncTTSRequest → 加入pending队列 → 立即返回
Tick() → 检查pending队列 → 发起API请求 → 更新状态为Requesting
Tick() → 检查Requesting状态 → API完成 → 播放音频 → 更新状态为Playing
Tick() → 检查Playing状态 → 播放完成 → 更新状态为Completed → 清理
```

**Tick处理逻辑**:
```cpp
void TTSManager::ProcessAsyncTTS() {
    // 处理当前请求（串行：同一时间只处理一个请求）
    if (hasCurrentRequest_ && !asyncPendingQueue_.empty()) {
        AsyncTTSRequest& req = asyncPendingQueue_.front();
        switch (req.state) {
        case AsyncTTSState::Pending:
            ProcessPendingRequest(req);   // 发起API请求
            break;
        case AsyncTTSState::Requesting:
            ProcessRequestingState(req);  // 检查API响应
            break;
        case AsyncTTSState::Playing:
            ProcessPlayingState(req);     // 检查播放完成
            break;
        case AsyncTTSState::Completed:
        case AsyncTTSState::Failed:
            // 清理已完成/失败的请求
            asyncPendingQueue_.pop_front();
            hasCurrentRequest_ = false;
            break;
        }
    }

    // 如果当前没有请求，从队列取下一个
    if (!hasCurrentRequest_ && !asyncPendingQueue_.empty()) {
        asyncPendingQueue_.front().startTime = std::chrono::steady_clock::now();
        hasCurrentRequest_ = true;
    }
}
```

**修改文件范围**:
- `TextToSpeech.h`: 添加AsyncTTSState枚举和AsyncTTSRequest结构，使用hasCurrentRequest_标志跟踪当前请求
- `TextToSpeech.cpp`: 重写SpeakWithMimo为异步模式，修改ProcessAsyncTTS处理逻辑，支持MCI设备错误处理
- `MimoTTSClient.h/cpp`: 修改RequestTTS支持异步回调
- `AudioPlayer.h/cpp`: 添加异步播放完成通知机制，处理MCI设备错误（返回true表示播放"完成"）

**超时设置**:
- API请求超时：5秒
- 音频播放超时：3秒（使用MCI状态+时间双重判断，处理设备错误）
- 最大重试次数：0次（禁用重试，避免重复播放）

**并发处理策略（串行排队）**:
```
请求队列结构：
pendingQueue: [请求1, 请求2, 请求3, ...]
currentRequest: 当前处理中的请求（最多1个）

处理流程：
if (currentRequest == nullptr && !pendingQueue.empty()) {
    currentRequest = pendingQueue.front();
    pendingQueue.pop();
    StartAPIRequest(currentRequest);
}

if (currentRequest != nullptr) {
    ProcessCurrentRequest();
    if (currentRequest.state == Completed) {
        currentRequest = nullptr;  // 完成，准备处理下一个
    }
}
```

**队列管理策略**:
| 策略项 | 决策 | 理由 |
|--------|------|------|
| 并发数 | 1个 | 简单，避免音频混合复杂性 |
| 队列长度 | 不限制 | 尽可能保留所有弹幕请求 |
| 队列溢出 | 无（保留所有） | 弹幕丢失可能引起用户困惑 |
| 排序方式 | FIFO（先进先出） | 简单公平 |
| 超时处理 | 标记为完成，继续处理下一个 | 避免卡死，使用时间+MCI双重判断 |

### 线程退出策略

**设计原则**: 程序退出时尽快退出，不需要等待所有任务完成

**实现方式**:
- WebSocket接收线程使用 `std::thread().detach()`（已有）
- 主线程退出时，分离线程由操作系统自动终止
- 异步TTS在主线程Tick中处理，主线程退出时自然停止
- 不需要复杂的同步和等待机制

**退出流程**:
```
程序退出请求
├── 主线程退出消息循环
├── 分离线程（WebSocket接收）自动终止
└── 异步TTS处理自然停止（在主线程）

不执行：
├── 不等待当前TTS播放完成
├── 不等待队列中的任务
└── 不清理临时文件（由操作系统回收）
```

**代码示例**:
```cpp
// 分离线程（主线程退出时自动终止）
std::thread([...](){
    while (running) {
        // 处理逻辑
    }
}).detach();

// 程序退出
void Destroy() {
    // 直接退出，不等待任何任务
}
```

## C++/CLI互操作设计

### P/Invoke 导出函数设计（DataBridgeExports）

**设计原则**: 使用 `__declspec(dllexport)` 导出函数，C# 通过 DllImport 调用

**实现方式**:
```cpp
// DataBridgeExports.h
typedef void(__stdcall* OnDanmuProcessedCallback)(const char* userName, const char* monsterName, void* userData);

extern "C" {
    __declspec(dllexport) void __stdcall DanmuProcessor_ProcessDanmu(const char* jsonStr);
    __declspec(dllexport) void __stdcall DataBridge_SetDanmuProcessedCallback(OnDanmuProcessedCallback callback, void* userData);
}
```

```cpp
// DataBridgeExports.cpp
static OnDanmuProcessedCallback g_danmuProcessedCallback = nullptr;

__declspec(dllexport) void __stdcall DataBridge_SetDanmuProcessedCallback(
    OnDanmuProcessedCallback callback, void* userData)
{
    g_danmuProcessedCallback = callback;
    g_danmuProcessedUserData = userData;

    if (callback != nullptr)
    {
        DanmuProcessor::Inst()->AddDanmuProcessedListener([](const DanmuProcessResult& result) {
            if (g_danmuProcessedCallback != nullptr && result.addedToQueue)
            {
                g_danmuProcessedCallback(
                    result.userName.c_str(), 
                    result.monsterName.c_str(), 
                    g_danmuProcessedUserData);
            }
        });
    }
}
```

### C# P/Invoke 声明设计（NativeImports）

**实现方式**:
```csharp
// NativeImports.cs
internal static class NativeImports
{
    private const string DllName = "MonsterOrderWilds.exe";

    [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
    public static extern void DanmuProcessor_ProcessDanmu(string jsonStr);

    [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
    public static extern void DataBridge_SetDanmuProcessedCallback(
        DanmuProcessedCallback callback, IntPtr userData);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]
    public delegate void DanmuProcessedCallback(string userName, string monsterName, IntPtr userData);
}
```

### C# 回调处理设计（DanmuManager）

**实现方式**:
```csharp
// DanmuManager.cs
internal class DanmuManager
{
    private static NativeImports.DanmuProcessedCallback danmuProcessedCallback;

    public DanmuManager()
    {
        danmuProcessedCallback = OnDanmuProcessed;
        NativeImports.DataBridge_SetDanmuProcessedCallback(danmuProcessedCallback, IntPtr.Zero);
    }

    public void OnReceivedRawJson(String rawJson)
    {
        NativeImports.DanmuProcessor_ProcessDanmu(rawJson);
    }

    private static void OnDanmuProcessed(string userName, string monsterName, IntPtr userData)
    {
        GlobalEventListener.Invoke("RefreshOrder", null);
        GlobalEventListener.Invoke("AddRollingInfo", new RollingInfo(
            userName + " 点怪 " + monsterName + " 成功！", Colors.Yellow));
    }
}
```

### 数据变更通知机制（C++调用C#回调）

```
流程：
C++数据处理完成 → DanmuProcessResult.addedToQueue=true 
    → 调用注册的回调 → C# OnDanmuProcessed 
    → GlobalEventListener.Invoke → UI刷新
```

### 路由角色设计（DanmuManager）

**核心洞察**: DanmuManager 是纯路由，不做业务逻辑

```
┌─────────────────────────────────────────────────────────────┐
│  DanmuManager (C#)                                          │
│                                                              │
│  - OnReceivedRawJson: 接收JSON，转发给C++                    │
│  - OnDanmuProcessed: 接收回调，触发UI刷新                    │
│  - 不做：JSON解析、业务逻辑、队列管理                         │
└─────────────────────────────────────────────────────────────┘
```

## 错误处理策略

### API失败重试
- MiMo TTS API失败：最多重试1次
- 重试失败：降级到SAPI

### 网络断开处理
- 检测到网络问题：自动降级到SAPI
- 网络恢复：不自动恢复MiMo（需手动刷新配置）

### API限流处理
- 收到429限流错误：立即降级到SAPI
- 不等待重试

## 配置管理策略

### 配置热更新
- 方式：手动生效
- 流程：用户修改配置 → 点击"应用"按钮 → 通知C++层更新 → 配置生效

## 边界条件处理

### TTS队列管理
- 队列长度：不限制（尽可能保留所有请求）
- 注意：需监控内存使用

### 日志记录
- 策略：保持现有方案（LOG_INFO、LOG_ERROR等宏）

## C#层数据绑定

### UI更新机制
- 方式：INotifyPropertyChanged
- 代理类实现该接口，支持WPF自动数据绑定

## UI阻塞优化策略

### 已识别的阻塞点
| 位置 | 阻塞类型 | 优化方案 |
|------|----------|----------|
| `SaveConfig` | 文件I/O在UI线程 | 异步保存（Task.Run/async-await） |
| `LoadConfig` | 文件I/O在UI线程 | 保持同步（启动时加载，可接受） |
| `MonsterData.LoadJsonData` | 文件I/O + 正则编译 | 异步加载（后台加载，完成后再启用功能） |
| `RefreshOrder` + `SortQueue` | 排序 + 列表重建 | 后台排序 + UI更新 |
| `Dispatcher.Invoke` (7处) | 同步调用可能阻塞 | 逐个评估 |

### 详细优化方案

**SaveConfig异步保存**:
```csharp
private async void SaveSettingsButton_Click(object sender, RoutedEventArgs e)
{
    await Task.Run(() => {
        ToolsMain.GetConfigService().SaveConfig();
    });
    // UI反馈
}
```

**MonsterData异步加载**:
```csharp
public async Task LoadJsonDataAsync()
{
    await Task.Run(() => {
        // JSON解析和正则编译在后台执行
    });
    // 加载完成，启用点怪功能
}
```

**RefreshOrder后台排序**:
```csharp
public async void RefreshOrder()
{
    var sortedQueue = await Task.Run(() => {
        PriorityQueue.GetInst().SortQueue();
        return PriorityQueue.GetInst().Queue.ToList();
    });
    
    // UI线程更新列表
    Dispatcher.InvokeAsync(new Action(delegate {
        MainList.Items.Clear();
        foreach (var item in sortedQueue) {
            MainList.Items.Add(item);
        }
    }));
}
```

**Dispatcher.Invoke评估**:
- 第90行 `Hide()` → 改为InvokeAsync
- 第101行 `Show()` → 改为InvokeAsync
- 第230行 `Show()` → 改为InvokeAsync
- 第237行 `SetStatus()` → 改为InvokeAsync
- 第248行 `SetStatus()` → 改为InvokeAsync
- 第259行 `SetStatus()` → 改为InvokeAsync
- 第270行 `OnHotKeyLock()` → 改为InvokeAsync

**决策**：所有7处 `Dispatcher.Invoke` 全部改为 `Dispatcher.InvokeAsync`

**理由**：
- 都是UI更新操作，不需要返回值
- 调用者均来自非UI线程，无需等待操作完成
- 改为异步可避免阻塞调用线程，提高响应性

## Hotkey用法说明设计

### UI位置
- 文件：`ConfigWindow.xaml`
- 位置："其他设置"标签页，在"点怪窗口透明度"设置下方

### 设计内容
```xml
<Border Margin="8,16,8,8" Padding="12" 
        Background="#F5F5F5" CornerRadius="4">
    <StackPanel>
        <TextBlock Text="快捷键" 
                   FontFamily="Segoe UI Variable"
                   FontSize="14" FontWeight="SemiBold"
                   Foreground="#222"
                   Margin="0,0,0,8" />
        <StackPanel Orientation="Horizontal" Margin="0,4,0,0">
            <Border Background="#E0E0E0" CornerRadius="3" 
                    Padding="6,3" Margin="0,0,8,0">
                <TextBlock Text="Alt + ,"
                           FontFamily="Consolas"
                           FontSize="13"
                           Foreground="#333" />
            </Border>
            <TextBlock Text="锁定/解锁点怪窗口"
                       FontFamily="Segoe UI Variable"
                       FontSize="13"
                       Foreground="#555"
                       VerticalAlignment="Center"
                       Margin="0,0,12,0" />
            <Button Content="锁定窗口"
                    Name="LockWindowButton"
                    Click="LockWindowButton_Click"
                    Padding="12,4"
                    FontSize="13" />
        </StackPanel>
        <TextBlock Text="锁定后窗口透明且可穿透，方便查看游戏画面"
                   FontFamily="Segoe UI Variable"
                   FontSize="12"
                   Foreground="#777"
                   Margin="0,4,0,0"
                   TextWrapping="Wrap" />
    </StackPanel>
</Border>
```

### 显示效果
- 灰色背景卡片（#F5F5F5）
- 标题："快捷键"（粗体）
- 快捷键显示：`Alt + ,`（等宽字体，灰色背景按钮样式）
- 功能说明：锁定/解锁点怪窗口
- 操作按钮："锁定窗口"按钮，点击触发锁定/解锁
- 详细说明：锁定后窗口透明且可穿透，方便查看游戏画面

### 按钮行为
- 点击按钮调用 `LockWindowButton_Click` 事件处理
- 事件处理中调用 `OrderedMonsterWindow.OnHotKeyLock()` 切换锁定状态
- 按钮文字根据锁定状态变化："锁定窗口"/"解锁窗口"