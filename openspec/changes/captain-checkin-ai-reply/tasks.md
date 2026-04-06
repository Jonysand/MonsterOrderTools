# Captain CheckIn AI Reply - Tasks

## UnitTest 项目说明

### 单元测试架构

本项目 C++ 单元测试采用 **BliveManagerTests.vcxproj** 独立项目配置：

- 测试文件：`*Tests.cpp`
- 编译配置：UnitTest 模式编译，Release 模式排除
- 依赖：主项目完整编译环境

### 测试文件清单

| 测试文件 | 所属项目 | 状态 |
|---------|---------|------|
| `ProfileManagerTests.cpp` | BliveManagerTests.vcxproj (UnitTest) | ⚠️ 需主项目环境 |
| `AIChatProviderTests.cpp` | BliveManagerTests.vcxproj (UnitTest) | ⚠️ 需主项目环境 |
| `TTSProviderTests.cpp` | BliveManagerTests.vcxproj (UnitTest) | ⚠️ 需主项目环境 |
| `CaptainCheckInModuleTests.cpp` | MonsterOrderWilds.vcxproj (UnitTest) | ⚠️ 需主项目环境 |

### 运行单元测试

```bash
MSBuild.exe BliveManagerTests.vcxproj -p:Configuration=UnitTest -p:Platform=x64 -t:Build
MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=UnitTest -p:Platform=x64 -t:Build
```

---

## Task 1: 配置字段 - ENABLE_CAPTAIN_CHECKIN_AI + CHECKIN_TRIGGER_WORDS

**注意**：`AI_PROVIDER` 是 credential 字段，由 C++ `CredentialsManager` 独立管理（见 Task 2），不经过 ConfigManager。

### Files
- Modify: `MonsterOrderWilds/ConfigManager.h`
- Modify: `MonsterOrderWilds/ConfigManager.cpp`
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp`
- Modify: `JonysandMHDanmuTools/DataBridgeWrapper.h`
- Modify: `JonysandMHDanmuTools/ToolsMain.cs`

### Steps
- [ ] 在 `ConfigData` 结构体中添加 `enableCaptainCheckinAI` 字段（bool，默认 true）
- [ ] 在 `ConfigData` 结构体中添加 `checkinTriggerWords` 字段（string，默认 "打卡,签到"）
- [ ] 在 `LoadConfig()` 中添加 JSON 加载逻辑（ENABLE_CAPTAIN_CHECKIN_AI, CHECKIN_TRIGGER_WORDS）
- [ ] 在 `SaveConfig()` 中添加 JSON 保存逻辑
- [ ] 在 `ConfigFieldRegistry.cpp` 的 `RegisterAll()` 中注册字段
- [ ] **注册回调函数**：当 `enableCaptainCheckinAI` 变更时，调用 `CaptainCheckInModule::Instance()->SetEnabled()` 更新开关状态
- [ ] **注册回调函数**：当 `checkinTriggerWords` 变更时，调用 `CaptainCheckInModule::Instance()->SetTriggerWords()` 更新触发词
- [ ] 在 `ConfigManager.h` 中添加 `SetEnableCaptainCheckinAI()` 和 `SetCheckinTriggerWords()` 方法声明
- [ ] 在 `ConfigManager.cpp` 中实现 `SetEnableCaptainCheckinAI()` 和 `SetCheckinTriggerWords()` 方法
- [ ] 在 `DataBridgeWrapper.h` 的 `ConfigProxy` 中添加 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords` 属性
- [ ] 在 `ToolsMain.cs` 的 `ConfigChanged()` 中添加 `ENABLE_CAPTAIN_CHECKIN_AI` 处理逻辑

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 1.7T: ProfileManager - 单元测试

### Files
- Create: `MonsterOrderWilds/ProfileManagerTests.cpp`

### Steps
- [ ] 编写 `TestProfileManager_Init` 测试：验证数据库初始化
- [ ] 编写 `TestProfileManager_SaveAndGet` 测试：验证用户画像保存和获取
- [ ] 编写 `TestProfileManager_Delete` 测试：验证删除功能
- [ ] 编写 `TestProfileManager_RecordCheckin` 测试：验证打卡记录
- [ ] 编写 `TestProfileManager_CalculateContinuousDays` 测试：验证连续天数计算
- [ ] 编写 `TestProfileManager_JsonSerialization` 测试：验证 JSON 序列化/反序列化
- [ ] 在 vcxproj 中添加测试文件，条件排除 Release 模式

### 验证
- [ ] 编译验证：`MSBuild.exe BliveManagerTests.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增测试文件

---

## Task 2: 凭据字段 - AI_PROVIDER

### Files
- Modify: `MonsterOrderWilds/CredentialsManager.h`
- Modify: `MonsterOrderWilds/CredentialsManager.cpp`
- Modify: `scripts/credentials.json`

### Steps
- [ ] 在 `CredentialsManager.h` 中添加 `GetAI_PROVIDER()` 函数声明
- [ ] 在 `CredentialsManager.cpp` 中添加 `s_aiProvider` 静态变量
- [ ] 在 `LoadCredentials()` 的 JSON 解析中添加 `AI_PROVIDER` 字段
- [ ] 实现 `GetAI_PROVIDER()` 函数
- [ ] 在 `credentials.json` 模板中添加 `AI_PROVIDER` 字段

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认修改范围正确

---

## Task 2T: 凭据字段 AI_PROVIDER - 单元测试

### Files
- Create: `MonsterOrderWilds/CredentialsManagerTests.cpp`

### Steps
- [ ] 编写 `TestAI_PROVIDER_GetCredentials` 测试：验证 GetAI_PROVIDER 返回非空
- [ ] 编写 `TestAI_PROVIDER_ParseJson` 测试：验证 JSON 格式解析正确（chat_provider, chat_api_key, tts_provider, tts_api_key）
- [ ] 编写 `TestAI_PROVIDER_InvalidJson` 测试：验证无效 JSON 处理
- [ ] 在 vcxproj 中添加测试文件，条件排除 Release 模式

### 验证
- [ ] 编译验证：`MSBuild.exe BliveManagerTests.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增测试文件

---

## Task 2.5: AI Chat Provider - MiniMaxAIChatProvider

### Files
- Create: `MonsterOrderWilds/AIChatProvider.h`
- Create: `MonsterOrderWilds/AIChatProviderFactory.cpp`
- Create: `MonsterOrderWilds/MiniMaxAIChatProvider.cpp`
- Modify: `MonsterOrderWilds/CaptainCheckInModule.cpp`

### Steps
- [ ] 创建 `AIChatProvider.h`，定义 `IAIChatProvider` 接口和 `AIChatProviderFactory` 声明
- [ ] 创建 `AIChatProviderFactory.cpp`，实现 `AIChatProviderFactory::Create()` 工厂方法
- [ ] 创建 `MiniMaxAIChatProvider.cpp`，实现 MiniMax 文本对话 API 调用
- [ ] 在 `CaptainCheckInModule` 中通过 `IAIChatProvider` 接口调用
- [ ] 从 `GetAI_PROVIDER()` 获取完整 JSON，解析 `chat_provider` 和 `chat_api_key` 后创建对应 Provider
- [ ] AI 回复文本通过 `ITTSProvider` 进行 TTS 播报（见 Task 2.6）

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 2.5T: AI Chat Provider - 单元测试

### Files
- Create: `MonsterOrderWilds/AIChatProviderTests.cpp`

### Steps
- [ ] 编写 `TestAIChatProvider_Interface` 测试：验证 IAIChatProvider 接口存在
- [ ] 编写 `TestMiniMaxAIChatProvider_BuildRequest` 测试：验证请求体构建
- [ ] 编写 `TestMiniMaxAIChatProvider_ParseResponse` 测试：验证响应解析
- [ ] 在 vcxproj 中添加测试文件，条件排除 Release 模式

### 验证
- [ ] 编译验证：`MSBuild.exe BliveManagerTests.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增测试文件

---

## Task 2.6: TTS Provider 实现（必须实现 ITTSProvider 接口）

**重要**：必须实现完整的 ITTSProvider 接口，不能仅依赖现有 MimoTTSClient。

### Files
- Create: `MonsterOrderWilds/ITTSProvider.h`
- Create: `MonsterOrderWilds/SapiTTSProvider.cpp`
- Create: `MonsterOrderWilds/XiaomiTTSProvider.cpp`
- Create: `MonsterOrderWilds/MiniMaxTTSProvider.cpp`
- Create: `MonsterOrderWilds/TTSProviderFactory.cpp`
- Modify: `MonsterOrderWilds/CaptainCheckInModule.cpp`

### Steps
- [ ] 创建 `ITTSProvider.h`，定义 `ITTSProvider` 接口、`TTSRequest`/`TTSResponse` 结构体和 `TTSProviderFactory`
- [ ] 创建 `SapiTTSProvider.cpp`，实现 Windows SAPI 降级 Provider（当 API Key 为空或 Provider 不可用时回退）
- [ ] 创建 `XiaomiTTSProvider.cpp`，实现 Xiaomi TTS API 调用
- [ ] 创建 `MiniMaxTTSProvider.cpp`，实现 MiniMax TTS API 调用
- [ ] 创建 `TTSProviderFactory.cpp`，实现工厂类，根据 `AI_PROVIDER` JSON 创建对应 Provider
- [ ] 在 `CaptainCheckInModule` 中通过 `TTSProviderFactory` 创建 TTS Provider
- [ ] AI 回复文本通过 `ITTSProvider::RequestTTS()` 进行 TTS 播报
- [ ] **关键**：AI 回复播报受 `enableVoice` 配置影响，关闭后不播报

**TTS API 实现说明**：
- Xiaomi TTS API 复用 `MimoTTSClient` 的请求/响应逻辑
- MiniMax TTS API 使用 `api.minimaxi.com/v1/t2a_v2` 端点
- 参考 `tts-provider-spec.md` 中的详细 API 规格

**注意**：
- `TTSManager` 会自动处理引擎回退（MiMo → SAPI），AI 回复始终被播报
- 如果 TTS 全部失败，记录错误日志但不阻塞流程

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 2.6T: TTS Provider - 单元测试

### Files
- Create: `MonsterOrderWilds/TTSProviderTests.cpp`

### Steps
- [ ] 编写 `TestITTSProvider_Interface` 测试：验证 ITTSProvider 接口存在
- [ ] 编写 `TestSapiTTSProvider_IsAvailable` 测试：验证 SapiTTSProvider 始终可用
- [ ] 编写 `TestXiaomiTTSProvider_BuildRequest` 测试：验证 Xiaomi TTS 请求体构建
- [ ] 编写 `TestMiniMaxTTSProvider_BuildRequest` 测试：验证 MiniMax TTS 请求体构建
- [ ] 编写 `TestTTSProviderFactory_Create` 测试：验证工厂创建正确 Provider
- [ ] 在 vcxproj 中添加测试文件，条件排除 Release 模式

### 验证
- [ ] 编译验证：`MSBuild.exe BliveManagerTests.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增测试文件

---

## Task 3: C# 配置层 - 数据结构

**注意**：`AI_PROVIDER` 是 credential 字段，由 C++ `CredentialsManager` 独立处理，不经过 C# 配置层。

### Files
- Modify: `JonysandMHDanmuTools/DataStructures.cs`
- Modify: `JonysandMHDanmuTools/Utils.cs`
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs`

### Steps
- [ ] 在 `ConfigDataSnapshot` 中添加 `EnableCaptainCheckinAI` 字段（bool）
- [ ] 在 `ConfigDataSnapshot` 中添加 `CheckinTriggerWords` 字段
- [ ] 在 `FromMainConfig()` 中映射 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords`
- [ ] 在 `ApplyTo()` 中映射字段
- [ ] 在 `ConfigFieldRegistry` 静态构造函数中注册 `enableCaptainCheckinAI` 和 `checkinTriggerWords`
- [ ] 在 `MainConfig` 类中添加 `ENABLE_CAPTAIN_CHECKIN_AI` 和 `CHECKIN_TRIGGER_WORDS` 属性
- [ ] 在 `ConfigProxy` 类中添加 `EnableCaptainCheckinAI` 和 `CheckinTriggerWords` 属性
- [ ] 在 `RefreshFromConfig()` 和 `ApplyToConfig()` 中处理字段

### 验证
- [ ] 编译验证：`MSBuild.exe JonysandMHDanmuTools.csproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认修改范围正确

---

## Task 3T: C# 配置层 - 数据结构单元测试

### Files
- Create: `JonysandMHDanmuTools.Tests/ConfigCheckinTriggerWordsTests.cs`

### Steps
- [ ] 编写 `TestConfigDataSnapshot_CheckinTriggerWords` 测试：验证字段存在和默认值
- [ ] 编写 `TestConfigProxy_CheckinTriggerWords` 测试：验证 Proxy 属性读写
- [ ] 编写 `TestConfigFieldRegistry_CheckinTriggerWords` 测试：验证字段注册
- [ ] 在 csproj 中添加测试文件

### 验证
- [ ] 编译验证：`dotnet build JonysandMHDanmuTools.Tests/JonysandMHDanmuTools.Tests.csproj`
- [ ] Git diff 检查：确认新增测试文件

---

## Task 4: C# 配置层 - UI（独立 Tab）

### Files
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml`
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs`

### Steps
- [ ] 在 ConfigWindow.xaml 中新增独立的 "舰长打卡AI" TabItem
- [ ] 在 "舰长打卡AI" Tab 中添加：
  - 总开关 CheckBox（绑定 EnableCaptainCheckinAI）
  - 触发词配置 TextBox（绑定 CheckinTriggerWords）
- [ ] 在 ConfigWindow.xaml.cs 中添加控件事件处理
- [ ] 在 `FillConfig()` 中初始化控件值
- [ ] 在 `SaveConfig()` 中保存控件值

### UI 布局示例

```xml
<TabItem Header="舰长打卡AI">
    <StackPanel Margin="10">
        <!-- 总开关 -->
        <CheckBox x:Name="EnableCaptainCheckinAICheckBox" 
                  Content="开启舰长打卡AI功能"
                  Margin="0,0,0,10"/>
        
        <!-- 触发词配置 -->
        <TextBlock Text="打卡触发词（逗号分隔）：" Margin="0,0,0,5"/>
        <TextBox x:Name="CheckinTriggerWordsTextBox" 
                 Width="300" 
                 HorizontalAlignment="Left"/>
        
        <TextBlock Text="默认：打卡,签到" 
                   Foreground="Gray" 
                   Margin="0,5,0,0"/>
    </StackPanel>
</TabItem>
```

### 验证
- [ ] 编译验证：`MSBuild.exe JonysandMHDanmuTools.csproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认修改范围正确

---

## Task 5: C++ 核心模块 - CaptainCheckInModule

### Files
- Create: `MonsterOrderWilds/CaptainCheckInModule.h`
- Create: `MonsterOrderWilds/CaptainCheckInModule.cpp`

### Steps
- [ ] 创建头文件，定义数据结构和方法
- [ ] 实现 `Init()` - 初始化 cppjieba，向 DanmuProcessor 注册舰长弹幕回调（ProfileManager 已在单独步骤初始化）
- [ ] 实现 `Destroy()` - 保存所有画像到数据库，注销事件监听
- [ ] 实现 `SetEnabled(bool)` - 设置模块开关状态
- [ ] 实现 `IsEnabled()` - 获取模块开关状态
- [ ] 实现 `ShouldLearn()` - 防刷屏检查（时间窗口、内容去重）
- [ ] 实现 `PushDanmuEvent()` - 学习弹幕入口
- [ ] 实现 `UpdateDanmuHistory()` - 更新弹幕历史
- [ ] 实现 `ExtractKeywords()` - cppjieba 分词提取关键词
- [ ] 实现 `UpdateKeywordFrequency()` - 更新关键词频率
- [ ] 实现 `LoadProfileFromDb()` - 从数据库加载画像到内存缓存
- [ ] 实现 `SaveProfileToDb()` - 异步保存画像到数据库
- [ ] 实现 `GenerateCheckinAnswerAsync()` - 异步生成回复 + TTS 播报
- [ ] 实现 `GenerateCheckinAnswerSync()` - 同步生成回复
- [ ] 实现 `IsCheckinMessage()` - 检测打卡消息
- [ ] 实现 `CalculateContinuousDays()` - 调用 ProfileManager 计算连续天数
- [ ] 实现 `GenerateAIAnswer()` - 构建Prompt，调用MiniMax文本对话API
- [ ] 实现 `BuildPrompt()` - 包含空关键词/历史发言的回退处理

**DanmuProcessor 舰长弹幕回调订阅**：
```cpp
void CaptainCheckInModule::Init() {
    // ... 初始化 cppjieba, ProfileManager
    
    // 注册舰长弹幕事件监听
    DanmuProcessor::Inst()->AddCaptainDanmuListener([this](const CaptainDanmuEvent& event) {
        if (IsEnabled()) {
            PushDanmuEvent(event);
        }
    });
}
```

**ProfileManager 集成说明**：
- 使用 `ProfileManager::Instance()` 获取单例
- 调用 `ProfileManager::GetProfile()` 按需加载画像
- 调用 `ProfileManager::SaveProfile()` 持久化画像
- 调用 `ProfileManager::CalculateContinuousDays()` 计算连续天数

**cppjieba 集成说明**：
- 使用 `cppjieba::Jieba` 类，初始化时传入词典路径
- 调用 `jieba_.Cut(content, words, true)` 进行分词（MixSegment 模式）
- 过滤条件：词长 >= 2、非停用词、非单字

**TTS 播报集成说明**：
- AI 回复文本通过 `TTSProviderFactory` 创建 `ITTSProvider` 进行 TTS 播报
- 使用 `AI_PROVIDER` 中的 `tts_provider` 和 `tts_api_key`
- AI 回复播报受 `enableVoice` 配置影响
- **必须调用 `TTSCacheManager::SaveCheckinAudio()` 缓存 AI TTS 音频**
- 缓存命名格式：`打卡_{username}_{timestamp}.mp3`
- 参考 `TextToSpeech.cpp` 中的缓存调用方式

**MiniMax API 调用说明**：
- **必须使用 WinHTTP 同步实现**，不能使用 `Network::MakeHttpsRequest`（协程函数）
- API Endpoint: `api.minimaxi.com`
- API Port: 443
- API Path: `/v1/text/chatcompletion_v2`
- Model: `M2-her`
- 请求体格式：
```json
{
  "model": "M2-her",
  "messages": [{"role": "user", "content": "{prompt}"}]
}
```
- 解析响应中的文本内容（`choices[0].message.content`）
- 参考 `MimoTTSClient.cpp` 中的 WinHTTP 同步实现方式
- [ ] 实现 `GetFallbackAnswer()` - 固定模板回复

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 5.5: CaptainCheckInModule 初始化导出函数

**重要**：需要在 `DataBridgeExports.cpp` 中添加 CaptainCheckInModule 的初始化导出函数，供 C# 层在 `DataBridge_Initialize()` 之后调用。

### 初始化顺序（必须严格遵守）
```
1. ProfileManager::Init()      → 初始化 SQLite 数据库
2. DanmuProcessor::Init()      → 初始化弹幕处理器
3. CaptainCheckInModule::Init() → 初始化 cppjieba，向 DanmuProcessor 注册舰长弹幕回调
```

**说明**：DanmuProcessor 先初始化，CaptainCheckInModule 后注册监听器。监听器在事件发生前注册即可。

### Files
- Modify: `MonsterOrderWilds/DataBridgeExports.cpp`

### Steps
- [ ] 在 `DataBridgeExports.cpp` 中添加 `CaptainCheckInModule_Initialize()` 导出函数
- [ ] 在导出函数中依次调用：
  1. `ProfileManager::Instance()->Init()`
  2. `DanmuProcessor::Inst()->Init()`
  3. `CaptainCheckInModule::Inst()->Init()`

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认修改范围正确

---

## Task 6: C++ 集成 - DanmuProcessor（事件解耦）

**重要**：DanmuProcessor 与 CaptainCheckInModule 通过 `CaptainDanmuHandler` 回调接口解耦，DanmuProcessor 不直接依赖 CaptainCheckInModule。

**说明**：C++ 层没有 `GlobalEventListener` 类（该类仅存在于 C# 层），需要使用观察者模式实现事件解耦。

### Files
- Modify: `MonsterOrderWilds/DanmuProcessor.cpp`
- Modify: `MonsterOrderWilds/DanmuProcessor.h`

### Steps
- [ ] 在 `DanmuProcessor.h` 中添加 `CaptainDanmuEvent` 结构体定义
- [ ] 在 `DanmuProcessor.h` 中添加 `CaptainDanmuHandler` 回调类型定义
- [ ] 在 `DanmuProcessor.h` 中添加 `AddCaptainDanmuListener()` 方法声明
- [ ] 在 `DanmuProcessor.h` 中添加 `captainDanmuListeners_` 成员变量
- [ ] 在 `DanmuProcessor.cpp` 中实现 `AddCaptainDanmuListener()` 方法
- [ ] 在 `DanmuProcessor.cpp` 中实现 `NotifyCaptainDanmu()` 方法
- [ ] 在 `DanmuProcessor::ProcessDanmu()` 中处理舰长弹幕（guard_level >= 3），调用 `NotifyCaptainDanmu()` 发出事件
- [ ] **不要**在 DanmuProcessor 中 include CaptainCheckInModule.h
- [ ] 在 `CaptainCheckInModule::Init()` 中注册事件监听（见 Task 5.5）

**DanmuProcessor 职责**：
- 只负责检测舰长弹幕并通过回调发出事件
- 不负责学习、统计、AI回复等逻辑

**CaptainCheckInModule 职责**（见 Task 5 和 Task 5.5）：
- 订阅 `CaptainDanmu` 事件
- 处理学习、打卡检测、AI回复等逻辑

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认修改范围正确（DanmuProcessor 不应依赖 CaptainCheckInModule.h）

---

## Task 7: C++ 集成 - vcxproj

### Files
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj.filters`

### Steps
- [ ] 将 `CaptainCheckInModule.h` 和 `CaptainCheckInModule.cpp` 添加到项目（Filter: DataProcessing）
- [ ] 将 `ProfileManager.h` 和 `ProfileManager.cpp` 添加到项目（Filter: DataProcessing）
- [ ] 将 `AIChatProvider.h` 和 `MiniMaxAIChatProvider.cpp` 添加到项目（Filter: DataProcessing）
- [ ] 将 `AIChatProviderFactory.cpp` 添加到项目（Filter: DataProcessing）
- [ ] 将 `ITTSProvider.h`、`SapiTTSProvider.cpp`、`XiaomiTTSProvider.cpp`、`MiniMaxTTSProvider.cpp`、`TTSProviderFactory.cpp` 添加到项目（Filter: DataProcessing）
- [x] ~~添加 sqlite_orm 头文件包含路径~~ → 已改用 SQLite C API，添加 `$(ProjectDir)..\external` 包含路径
- [ ] 添加 cppjieba 头文件包含路径 `$(ProjectDir)cppjieba\include`
- [ ] 添加到 vcxproj 的 Header Files 和 Source Files 中
- [ ] **同时更新 .vcxproj.filters 文件**，在对应的 `<Filter>` 节点下添加新文件（DataProcessing）
- [ ] 添加条件排除（Release模式排除测试文件）

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 8: CaptainCheckInModule 单元测试

### Files
- Create: `MonsterOrderWilds/CaptainCheckInModuleTests.cpp`
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`

### Steps
- [ ] 编写 `TestPushDanmuEvent_Basic` 测试：验证基本学习功能
- [ ] 编写 `TestPushDanmuEvent_TimeWindow` 测试：验证时间窗口防刷屏
- [ ] 编写 `TestPushDanmuEvent_ContentDeduplication` 测试：验证内容去重
- [ ] 编写 `TestIsCheckinMessage` 测试：验证打卡检测
- [ ] 编写 `TestIsCheckinMessage_CustomTriggerWords` 测试：验证自定义触发词
- [ ] 编写 `TestCalculateContinuousDays` 测试：验证连续天数计算
- [ ] 编写 `TestExtractKeywords` 测试：验证关键词提取
- [ ] 编写 `TestGetFallbackAnswer` 测试：验证固定模板回复
- [ ] 编写 `TestGenerateAIAnswer_PromptBuilding` 测试：验证 Prompt 构建
- [ ] 在 vcxproj 中添加测试文件，条件排除 Release 模式

### 验证
- [ ] 编译验证：`MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查：确认新增文件和修改范围正确

---

## Task 9: 安装包检查

### Files
- Check: `openspec/changes/installer/.openspec.yaml`
- Check: `installer/` 目录下的打包配置

### Steps
- [ ] 检查是否需要添加新的配置文件到安装包
- [ ] 如需要，更新安装包配置

**必须打包到安装包的文件**：
| 文件 | 目标路径 |
|------|----------|
| `dict/jieba.dict.utf8` | `{AppRoot}/dict/jieba.dict.utf8` |
| `dict/hmm_model.utf8` | `{AppRoot}/dict/hmm_model.utf8` |
| `dict/stop_words.utf8` | `{AppRoot}/dict/stop_words.utf8` |
| `dict/弹幕习惯词黑白名单配置.txt` | `{AppRoot}/dict/弹幕习惯词黑白名单配置.txt` |

**注意**：
- `captain_profiles.db` 在运行时自动创建，无需打包
- `弹幕习惯词黑白名单配置.txt` 是配置说明文档

### 验证
- [ ] Git diff 检查：确认安装包配置修改范围正确
