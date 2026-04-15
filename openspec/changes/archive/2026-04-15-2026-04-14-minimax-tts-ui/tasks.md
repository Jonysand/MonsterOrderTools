# MiniMax TTS UI 配置实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 在 UI 上添加 MiniMax TTS 配置界面（音色选择 + 语速滑块），并修复 hex 解码 bug

**Architecture:** 扩展现有 TTS 配置层，添加 MiniMax 专用配置字段，在 ConfigWindow 中添加独立配置面板

**Tech Stack:** C++ (ConfigManager, ConfigFieldRegistry), C# (ConfigWindow, Utils, ProxyClasses)

---

## 1. C++ 层配置字段添加

### Task 1: ConfigData 添加 MiniMax 字段

**Files:**
- Modify: `MonsterOrderWilds/ConfigManager.h:23-43`

- [x] **Step 1: 在 ConfigData 结构体中添加字段**

在 `// MiMo TTS 配置` 部分后添加：
```cpp
    // MiniMax TTS 配置
    std::string minimaxVoiceId = "female-tianmei";
    float minimaxSpeed = 1.5f;
```

- [x] **Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 2: ConfigFieldRegistry 注册 MiniMax 字段

**Files:**
- Modify: `MonsterOrderWilds/ConfigFieldRegistry.cpp:60-79`

- [x] **Step 1: 添加 MiniMax 字段注册**

在 `REGISTER_FIELD("mimoSpeed", float, mimoSpeed, ConfigFieldType::Float);` 后添加：
```cpp
    REGISTER_FIELD("minimaxVoiceId", std::string, minimaxVoiceId, ConfigFieldType::String);
    REGISTER_FIELD("minimaxSpeed", float, minimaxSpeed, ConfigFieldType::Float);
```

- [x] **Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 3: MiniMaxTTSProvider Hex 解码修复

**Files:**
- Modify: `MonsterOrderWilds/MiniMaxTTSProvider.cpp:103-172`

- [x] **Step 1: 添加 HexToBytes 函数**

在 `Base64ToBytes` 函数后添加：
```cpp
std::vector<uint8_t> MiniMaxTTSProvider::HexToBytes(const std::string& hex) const {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t byte = (uint8_t)std::strtol(byteStr.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}
```

- [x] **Step 2: 修改 ParseResponse 使用 HexToBytes**

在 `ParseResponse` 函数中，将：
```cpp
result.audioData = Base64ToBytes(audioBase64);
```
改为：
```cpp
result.audioData = HexToBytes(audioBase64);
```

- [x] **Step 3: 在头文件中声明 HexToBytes**

修改 `MonsterOrderWilds/TTSProvider.h` 中 `MiniMaxTTSProvider` 类，添加：
```cpp
std::vector<uint8_t> HexToBytes(const std::string& hex) const;
```

- [x] **Step 4: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 4: MiniMaxTTSProvider 单元测试添加

**Files:**
- Modify: `MonsterOrderWilds/TTSProviderTests.cpp:171-193`

- [x] **Step 1: 添加 HexToBytes 测试**

在 `TestMiniMaxTTSProvider_ParseResponse_Success` 后添加：
```cpp
void TestMiniMaxTTSProvider_HexToBytes()
{
    MiniMaxTTSProvider minimax("test_key");
    
    // "4849" hex = 'H'(0x48) 'I'(0x49)
    auto result = minimax.HexToBytes("4849");
    assert(result.size() == 2);
    assert(result[0] == 0x48);
    assert(result[1] == 0x49);
    
    // "616263" hex = 'a'(0x61) 'b'(0x62) 'c'(0x63)
    result = minimax.HexToBytes("616263");
    assert(result.size() == 3);
    assert(result[0] == 0x61);
    assert(result[1] == 0x62);
    assert(result[2] == 0x63);
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_HexToBytes" << std::endl;
}

void TestMiniMaxTTSProvider_ParseResponse_WithHexAudio()
{
    MiniMaxTTSProvider minimax("test_key");
    // "4849" is hex encoded "HI"
    std::string responseBody = "{\"data\":{\"audio\":\"4849\",\"status\":2}}";
    
    auto resp = minimax.ParseResponse(responseBody);
    assert(resp.success == true);
    // Hex "4849" = bytes [0x48, 0x49] = 'H', 'I'
    assert(resp.audioData.size() == 2);
    assert(resp.audioData[0] == 0x48);  // 'H'
    assert(resp.audioData[1] == 0x49);  // 'I'
    
    std::cout << "[PASS] TestMiniMaxTTSProvider_ParseResponse_WithHexAudio" << std::endl;
}
```

- [x] **Step 2: 在 RunTTSProviderTests 中注册新测试**

在 `TestMiniMaxTTSProvider_ParseResponse_Error();` 后添加：
```cpp
    TestMiniMaxTTSProvider_HexToBytes();
    TestMiniMaxTTSProvider_ParseResponse_WithHexAudio();
```

- [x] **Step 3: 验证编译并运行测试**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## 2. C# 层配置字段添加

### Task 5: DataStructures.cs 添加 MiniMax 字段

**Files:**
- Modify: `JonysandMHDanmuTools/DataStructures.cs:24-46`

- [x] **Step 1: 在 ConfigDataSnapshot 中添加字段**

在 `// MiMo TTS 配置` 部分后添加：
```csharp
        // MiniMax TTS 配置
        public string MinimaxVoiceId;
        public float MinimaxSpeed;
```

- [x] **Step 2: 在 FromMainConfig 中添加映射**

在 `.MimoSpeed = config.MIMO_SPEED,` 后添加：
```csharp
                MinimaxVoiceId = config.MINIMAX_VOICE_ID ?? "female-tianmei",
                MinimaxSpeed = config.MINIMAX_SPEED,
```

- [x] **Step 3: 在 ApplyTo 中添加映射**

在 `config.MIMO_SPEED = MimoSpeed;` 后添加：
```csharp
            config.MINIMAX_VOICE_ID = MinimaxVoiceId;
            config.MINIMAX_SPEED = MinimaxSpeed;
```

- [x] **Step 4: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 6: Utils.cs 添加 MiniMax 配置属性

**Files:**
- Modify: `JonysandMHDanmuTools/Utils.cs:186-234`

- [x] **Step 1: 在 ConfigFieldRegistry 中注册 MiniMax 字段**

在 `Register("mimoSpeed", ...)` 后添加：
```csharp
            Register("minimaxVoiceId", ConfigFieldType.String,
                () => GetString("minimaxVoiceId"),
                v => SetValue("minimaxVoiceId", (string)v, ConfigFieldType.String));

            Register("minimaxSpeed", ConfigFieldType.Float,
                () => GetFloat("minimaxSpeed"),
                v => SetValue("minimaxSpeed", (float)v, ConfigFieldType.Float));
```

- [x] **Step 2: 在 MainConfig 类中添加属性**

在 `public float MIMO_SPEED` 属性后添加：
```csharp
        public String MINIMAX_VOICE_ID
        {
            get => (string)ConfigFieldRegistry.Get("minimaxVoiceId");
            set { ConfigFieldRegistry.Set("minimaxVoiceId", value); OnPropertyChanged(); }
        }

        public float MINIMAX_SPEED
        {
            get => (float)ConfigFieldRegistry.Get("minimaxSpeed");
            set { ConfigFieldRegistry.Set("minimaxSpeed", value); OnPropertyChanged(); }
        }
```

- [x] **Step 3: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 7: ProxyClasses.cs 添加 MiniMax 属性

**Files:**
- Modify: `JonysandMHDanmuTools/ProxyClasses.cs:131-178`

- [x] **Step 1: 在 ConfigProxy 中添加 MiniMax 属性**

在 `public float MimoSpeed` 属性后添加：
```csharp
        private string _minimaxVoiceId = "female-tianmei";
        public string MinimaxVoiceId
        {
            get => _minimaxVoiceId;
            set { _minimaxVoiceId = value; OnPropertyChanged(); }
        }

        private float _minimaxSpeed = 1.0f;
        public float MinimaxSpeed
        {
            get => _minimaxSpeed;
            set { _minimaxSpeed = value; OnPropertyChanged(); }
        }
```

- [x] **Step 2: 在 RefreshFromConfig 中添加赋值**

在 `MimoSpeed = config.MIMO_SPEED;` 后添加：
```csharp
            MinimaxVoiceId = config.MINIMAX_VOICE_ID ?? "female-tianmei";
            MinimaxSpeed = config.MINIMAX_SPEED;
```

- [x] **Step 3: 在 ApplyToConfig 中添加赋值**

在 `config.MIMO_SPEED = MimoSpeed;` 后添加：
```csharp
            config.MINIMAX_VOICE_ID = MinimaxVoiceId;
            config.MINIMAX_SPEED = MinimaxSpeed;
```

- [x] **Step 4: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## 3. UI 层添加 MiniMax 配置面板

### Task 8: ConfigWindow.xaml 添加 MiniMax 配置面板

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml`

- [x] **Step 1: 在 TTSEngineComboBox 中添加 MiniMax 选项**

找到 `TTSEngineComboBox` 的定义，添加第 4 个选项：
```xml
<ComboBoxItem Content="MiniMax" Tag="minimax" />
```

- [x] **Step 2: 添加 MiniMaxConfigPanel**

在 `SapiConfigPanel` 后添加：
```xml
                            <Border Name="MiniMaxConfigPanel" Margin="8,8,0,0" Visibility="Visible">
                                <StackPanel>
                                    <Label Content="MiniMax 设置" FontWeight="SemiBold" Foreground="#0078D4" Margin="0,4,0,4" />
                                    
                                    <!-- 音色选择 -->
                                    <StackPanel Orientation="Horizontal" Margin="0,4,0,4">
                                        <Label Content="音色" VerticalAlignment="Center" Margin="0,0,8,0" />
                                        <ComboBox Name="MiniMaxVoiceComboBox" Width="200" 
                                                  SelectionChanged="MiniMaxVoiceComboBox_SelectionChanged">
                                            <ComboBoxItem Content="青涩青年音色" Tag="male-qn-qingse" IsSelected="True" />
                                            <ComboBoxItem Content="精英青年音色" Tag="male-qn-jingying" />
                                            <ComboBoxItem Content="霸道青年音色" Tag="male-qn-badao" />
                                            <ComboBoxItem Content="青年大学生音色" Tag="male-qn-daxuesheng" />
                                            <ComboBoxItem Content="少女音色" Tag="female-shaonv" />
                                            <ComboBoxItem Content="御姐音色" Tag="female-yujie" />
                                            <ComboBoxItem Content="成熟女性音色" Tag="female-chengshu" />
                                            <ComboBoxItem Content="甜美女性音色" Tag="female-tianmei" />
                                            <ComboBoxItem Content="青涩青年音色-beta" Tag="male-qn-qingse-jingpin" />
                                            <ComboBoxItem Content="精英青年音色-beta" Tag="male-qn-jingying-jingpin" />
                                            <ComboBoxItem Content="霸道青年音色-beta" Tag="male-qn-badao-jingpin" />
                                            <ComboBoxItem Content="青年大学生音色-beta" Tag="male-qn-daxuesheng-jingpin" />
                                            <ComboBoxItem Content="少女音色-beta" Tag="female-shaonv-jingpin" />
                                            <ComboBoxItem Content="御姐音色-beta" Tag="female-yujie-jingpin" />
                                            <ComboBoxItem Content="成熟女性音色-beta" Tag="female-chengshu-jingpin" />
                                            <ComboBoxItem Content="甜美女性音色-beta" Tag="female-tianmei-jingpin" />
                                            <ComboBoxItem Content="聪明男童" Tag="clever_boy" />
                                            <ComboBoxItem Content="可爱男童" Tag="cute_boy" />
                                            <ComboBoxItem Content="萌萌女童" Tag="lovely_girl" />
                                            <ComboBoxItem Content="卡通猪小琪" Tag="cartoon_pig" />
                                            <ComboBoxItem Content="病娇弟弟" Tag="bingjiao_didi" />
                                            <ComboBoxItem Content="俊朗男友" Tag="junlang_nanyou" />
                                            <ComboBoxItem Content="纯真学弟" Tag="chunzhen_xuedi" />
                                            <ComboBoxItem Content="冷淡学长" Tag="lengdan_xiongzhang" />
                                            <ComboBoxItem Content="霸道少爷" Tag="badao_shaoye" />
                                            <ComboBoxItem Content="甜心小玲" Tag="tianxin_xiaoling" />
                                            <ComboBoxItem Content="俏皮萌妹" Tag="qiaopi_mengmei" />
                                            <ComboBoxItem Content="妩媚御姐" Tag="wumei_yujie" />
                                            <ComboBoxItem Content="嗲嗲学妹" Tag="diadia_xuemei" />
                                            <ComboBoxItem Content="淡雅学姐" Tag="danya_xuejie" />
                                            <ComboBoxItem Content="沉稳高管" Tag="Chinese (Mandarin)_Reliable_Executive" />
                                            <ComboBoxItem Content="新闻女声" Tag="Chinese (Mandarin)_News_Anchor" />
                                            <ComboBoxItem Content="傲娇御姐" Tag="Chinese (Mandarin)_Mature_Woman" />
                                            <ComboBoxItem Content="不羁青年" Tag="Chinese (Mandarin)_Unrestrained_Young_Man" />
                                            <ComboBoxItem Content="嚣张小姐" Tag="Arrogant_Miss" />
                                            <ComboBoxItem Content="机械战甲" Tag="Robot_Armor" />
                                            <ComboBoxItem Content="热心大婶" Tag="Chinese (Mandarin)_Kind-hearted_Antie" />
                                            <ComboBoxItem Content="港普空姐" Tag="Chinese (Mandarin)_HK_Flight_Attendant" />
                                            <ComboBoxItem Content="搞笑大爷" Tag="Chinese (Mandarin)_Humorous_Elder" />
                                            <ComboBoxItem Content="温润男声" Tag="Chinese (Mandarin)_Gentleman" />
                                            <ComboBoxItem Content="温暖闺蜜" Tag="Chinese (Mandarin)_Warm_Bestie" />
                                            <ComboBoxItem Content="播报男声" Tag="Chinese (Mandarin)_Male_Announcer" />
                                            <ComboBoxItem Content="甜美女声" Tag="Chinese (Mandarin)_Sweet_Lady" />
                                            <ComboBoxItem Content="南方小哥" Tag="Chinese (Mandarin)_Southern_Young_Man" />
                                            <ComboBoxItem Content="阅历姐姐" Tag="Chinese (Mandarin)_Wise_Women" />
                                            <ComboBoxItem Content="温润青年" Tag="Chinese (Mandarin)_Gentle_Youth" />
                                            <ComboBoxItem Content="温暖少女" Tag="Chinese (Mandarin)_Warm_Girl" />
                                            <ComboBoxItem Content="花甲奶奶" Tag="Chinese (Mandarin)_Kind-hearted_Elder" />
                                            <ComboBoxItem Content="憨憨萌兽" Tag="Chinese (Mandarin)_Cute_Spirit" />
                                            <ComboBoxItem Content="电台男主播" Tag="Chinese (Mandarin)_Radio_Host" />
                                            <ComboBoxItem Content="抒情男声" Tag="Chinese (Mandarin)_Lyrical_Voice" />
                                            <ComboBoxItem Content="率真弟弟" Tag="Chinese (Mandarin)_Straightforward_Boy" />
                                            <ComboBoxItem Content="真诚青年" Tag="Chinese (Mandarin)_Sincere_Adult" />
                                            <ComboBoxItem Content="温柔学姐" Tag="Chinese (Mandarin)_Gentle_Senior" />
                                            <ComboBoxItem Content="嘴硬竹马" Tag="Chinese (Mandarin)_Stubborn_Friend" />
                                            <ComboBoxItem Content="清脆少女" Tag="Chinese (Mandarin)_Crisp_Girl" />
                                            <ComboBoxItem Content="清澈邻家弟弟" Tag="Chinese (Mandarin)_Pure-hearted_Boy" />
                                            <ComboBoxItem Content="柔和少女" Tag="Chinese (Mandarin)_Soft_Girl" />
                                        </ComboBox>
                                    </StackPanel>
                                    
                                    <!-- 语速滑块 -->
                                    <StackPanel Orientation="Horizontal" Margin="0,4,0,4">
                                        <Label Content="语速" VerticalAlignment="Center" Margin="0,0,8,0" />
                                        <Slider Name="MiniMaxSpeedSlider" Minimum="1.0" Maximum="2"
                                                Value="1.5" Width="120" TickFrequency="0.1"
                                                IsSnapToTickEnabled="True"
                                                ValueChanged="MiniMaxSpeedSlider_ValueChanged" />
                                        <TextBlock Text="{Binding ElementName=MiniMaxSpeedSlider, Path=Value, StringFormat=F1}" 
                                                   VerticalAlignment="Center" Margin="8,0,0,0" />
                                    </StackPanel>
                                </StackPanel>
                            </Border>
```

- [x] **Step 3: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 9: ConfigWindow.xaml.cs 添加事件处理

**Files:**
- Modify: `JonysandMHDanmuTools/ConfigWindow.xaml.cs`

- [x] **Step 1: 保持 TTSEngineComboBox_SelectionChanged 现有逻辑**

所有 TTS 配置面板（MiniMax、MiMo、SAPI）始终可见，无需修改面板切换逻辑。

- [x] **Step 2: 添加 MiniMaxVoiceComboBox_SelectionChanged 事件处理**

在 `MimoStyleComboBox_SelectionChanged` 后添加：
```csharp
private void MiniMaxVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
{
    var comboBox = sender as System.Windows.Controls.ComboBox;
    if (comboBox == null || comboBox.SelectedItem == null)
        return;
    var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
    if (selectedItem == null)
        return;
    string voiceId = selectedItem.Tag?.ToString() ?? "female-tianmei";
    GlobalEventListener.Invoke("ConfigChanged", $"MINIMAX_VOICE_ID:{voiceId}");
}
```

- [x] **Step 3: 添加 MiniMaxSpeedSlider_ValueChanged 事件处理**

在 `MiniMaxVoiceComboBox_SelectionChanged` 后添加：
```csharp
private void MiniMaxSpeedSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
{
    GlobalEventListener.Invoke("ConfigChanged", $"MINIMAX_SPEED:{e.NewValue}");
}
```

- [x] **Step 4: 在 FillConfig 中添加 MiniMax 配置初始化**

在 `// 设置语音风格（使用Tag属性）` 部分后添加：
```csharp
            // 设置 MiniMax 音色（使用Tag属性）
            for (int i = 0; i < MiniMaxVoiceComboBox.Items.Count; i++)
            {
                var item = MiniMaxVoiceComboBox.Items[i] as System.Windows.Controls.ComboBoxItem;
                if (item != null && item.Tag?.ToString() == config.MINIMAX_VOICE_ID)
                {
                    MiniMaxVoiceComboBox.SelectedIndex = i;
                    break;
                }
            }

            // 设置 MiniMax 语速
            MiniMaxSpeedSlider.Value = config.MINIMAX_SPEED;
```

- [x] **Step 5: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

### Task 10: ToolsMain.cs 添加 ConfigChanged 处理

**Files:**
- Modify: `JonysandMHDanmuTools/ToolsMain.cs`

- [x] **Step 1: 在 ConfigChanged 中添加 MiniMax 配置处理**

在处理 `TTS_ENGINE` 的部分后添加：
```csharp
            else if (parts[0] == "MINIMAX_VOICE_ID")
                _Config.Config.MINIMAX_VOICE_ID = parts[1];
            else if (parts[0] == "MINIMAX_SPEED")
            {
                if (float.TryParse(parts[1], out float speed))
                    _Config.Config.MINIMAX_SPEED = speed;
            }
```

- [x] **Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Debug -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## 验证与测试

### Task 11: 最终编译验证

- [x] **Step 1: Release 配置编译验证**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

### Task 12: 单元测试验证

- [x] **Step 1: 运行 TTSProvider 单元测试**

验证 MiniMaxTTSProvider 的 hex 解码测试通过

---

## 文件清单

| 文件 | 修改类型 |
|------|---------|
| `MonsterOrderWilds/ConfigManager.h` | 修改 - 添加 minimaxVoiceId, minimaxSpeed |
| `MonsterOrderWilds/ConfigFieldRegistry.cpp` | 修改 - 注册 MiniMax 字段 |
| `MonsterOrderWilds/MiniMaxTTSProvider.cpp` | 修改 - Hex 解码修复 |
| `MonsterOrderWilds/TTSProvider.h` | 修改 - 添加 HexToBytes 声明 |
| `MonsterOrderWilds/TTSProviderTests.cpp` | 修改 - 添加 hex 解码测试 |
| `JonysandMHDanmuTools/DataStructures.cs` | 修改 - 添加 Snapshot 字段 |
| `JonysandMHDanmuTools/Utils.cs` | 修改 - 添加配置属性 |
| `JonysandMHDanmuTools/ProxyClasses.cs` | 修改 - 添加 Proxy 属性 |
| `JonysandMHDanmuTools/ConfigWindow.xaml` | 修改 - 添加 MiniMax 配置面板 |
| `JonysandMHDanmuTools/ConfigWindow.xaml.cs` | 修改 - 添加事件处理 |
| `JonysandMHDanmuTools/ToolsMain.cs` | 修改 - 添加 ConfigChanged 处理 |
