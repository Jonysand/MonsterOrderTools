# MiniMax TTS UI 配置设计

## Context

MiniMax TTS 功能已实现但缺少 UI 配置界面。用户需要能够：
1. 选择 MiniMax 作为 TTS 引擎
2. 选择 MiniMax 音色（58个选项）
3. 调节语速（0.2~2）

当前 TTS 引擎选择仅支持"自动/小米MiMo/Windows SAPI"，需要添加 MiniMax 选项。

## Goals / Non-Goals

**Goals:**
- 添加 MiniMax TTS 引擎到下拉框选择
- 创建独立的 MiniMax 配置面板（音色选择 + 语速滑块）
- 修复 MiniMaxTTSProvider 的 hex 解码 bug
- 保持与现有 MiMo/SAPI 配置面板的一致性

**Non-Goals:**
- 不修改 MiniMax API 集成逻辑（仅修复解码 bug）
- 不添加 MiniMax 专属高级功能（如情绪控制）
- 不修改其他 TTS 引擎的现有功能

## Decisions

### 1. TTS 引擎选择扩展

TTSEngineComboBox 顺序调整（自动优先，MiniMax 第二）：

| Index | Content | Tag |
|-------|---------|-----|
| 0 | 自动 | auto |
| 1 | MiniMax | minimax |
| 2 | 小米MiMo | mimo |
| 3 | Windows SAPI | sapi |

### 2. MiniMax 配置面板结构

```xml
<Border Name="MiniMaxConfigPanel" Margin="8,8,0,0">
    <StackPanel>
        <Label Content="MiniMax 设置" FontWeight="SemiBold" Foreground="#0078D4" />
        
        <!-- 音色选择 -->
        <StackPanel Orientation="Horizontal" Margin="0,4,0,4">
            <Label Content="音色" VerticalAlignment="Center" />
            <ComboBox Name="MiniMaxVoiceComboBox" Width="200" 
                      SelectionChanged="MiniMaxVoiceComboBox_SelectionChanged">
                <!-- 58 个音色选项 -->
            </ComboBox>
        </StackPanel>
        
        <!-- 语速滑块 -->
        <StackPanel Orientation="Horizontal" Margin="0,4,0,4">
            <Label Content="语速" VerticalAlignment="Center" />
            <Slider Name="MiniMaxSpeedSlider" Minimum="1.0" Maximum="2" 
                    Value="1" Width="120" TickFrequency="0.1"
                    ValueChanged="MiniMaxSpeedSlider_ValueChanged" />
            <TextBlock Text="{Binding Value}" VerticalAlignment="Center" />
        </StackPanel>
    </StackPanel>
</Border>
```

### 3. 音色列表（58个）

使用 ComboBoxItem.Tag 存储 voice_id：

| 中文显示 | Tag (voice_id) |
|---------|----------------|
| 青涩青年音色 | male-qn-qingse |
| 精英青年音色 | male-qn-jingying |
| ... | ... |
| 柔和少女 | Chinese (Mandarin)_Soft_Girl |

### 4. 面板布局（所有面板始终显示）

所有语音设置面板始终可见，MiniMax 排在最前面：

```xml
<!-- TTS 引擎选择 -->
<ComboBox Name="TTSEngineComboBox">
    <ComboBoxItem Content="自动" Tag="auto" />
    <ComboBoxItem Content="MiniMax" Tag="minimax" />
    <ComboBoxItem Content="小米MiMo" Tag="mimo" />
    <ComboBoxItem Content="Windows SAPI" Tag="sapi" />
</ComboBox>

<!-- 面板顺序：MiniMax -> MiMo -> SAPI（全部始终可见）-->
<Border Name="MiniMaxConfigPanel" Visibility="Visible">
    <!-- MiniMax 设置面板 -->
</Border>
<Border Name="MimoConfigPanel" Visibility="Visible">
    <!-- MiMo 设置面板 -->
</Border>
<Border Name="SapiConfigPanel" Visibility="Visible">
    <!-- SAPI 设置面板 -->
</Border>
```

### 5. 配置字段映射

| JSON 配置 | C++ 字段 | C# 属性 | 默认值 |
|-----------|---------|---------|--------|
| MINIMAX_VOICE_ID | minimaxVoiceId | MinimaxVoiceId | female-tianmei |
| MINIMAX_SPEED | minimaxSpeed | MinimaxSpeed | 1.5 |

### 6. Hex 解码修复

**问题**: API 返回 hex 编码，但代码使用 Base64 解码

**修复**: 添加 `HexToBytes()` 函数

```cpp
std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t byte = (uint8_t)std::strtol(byteStr.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}
```

## Risks / Trade-offs

1. **音色列表过长**: 58 个选项可能导致下拉框过长，考虑使用分组或搜索功能（暂不实现，保持简单）
2. **与其他 TTS 共用 API Key**: MiniMax TTS 和 MiniMax Chat 使用不同的 API Key，需要确保用户配置正确的 Key
3. **滑块精度**: 0.2~2 范围，0.1 精度，可能需要验证实际效果

## Files to Modify

| File | Changes |
|------|---------|
| ConfigWindow.xaml | 添加 MiniMaxConfigPanel，扩展 TTSEngineComboBox |
| ConfigWindow.xaml.cs | 添加事件处理，FillConfig 更新 |
| Utils.cs | 添加 MINIMAX_SPEED, MINIMAX_VOICE_ID |
| DataStructures.cs | 添加 minimaxSpeed, minimaxVoiceId |
| ProxyClasses.cs | 添加 MinimaxSpeed, MinimaxVoiceId |
| ConfigManager.h | ConfigData 添加 minimaxSpeed, minimaxVoiceId |
| ConfigFieldRegistry.cpp | 注册新字段 |
| ConfigManager.cpp | LoadConfig/SaveConfig 处理 JSON |
| MiniMaxTTSProvider.cpp | 修复 hex 解码 |
| MiniMaxTTSProviderTests.cpp | 添加 hex 解码测试 |

## MiniMax Voice ID 列表

```
male-qn-qingse       - 青涩青年音色
male-qn-jingying     - 精英青年音色
male-qn-badao        - 霸道青年音色
male-qn-daxuesheng   - 青年大学生音色
female-shaonv        - 少女音色
female-yujie         - 御姐音色
female-chengshu      - 成熟女性音色
female-tianmei       - 甜美女性音色
male-qn-qingse-jingpin       - 青涩青年音色-beta
male-qn-jingying-jingpin     - 精英青年音色-beta
male-qn-badao-jingpin        - 霸道青年音色-beta
male-qn-daxuesheng-jingpin   - 青年大学生音色-beta
female-shaonv-jingpin        - 少女音色-beta
female-yujie-jingpin         - 御姐音色-beta
female-chengshu-jingpin      - 成熟女性音色-beta
female-tianmei-jingpin       - 甜美女性音色-beta
clever_boy           - 聪明男童
cute_boy             - 可爱男童
lovely_girl          - 萌萌女童
cartoon_pig          - 卡通猪小琪
bingjiao_didi        - 病娇弟弟
junlang_nanyou       - 俊朗男友
chunzhen_xuedi       - 纯真学弟
lengdan_xiongzhang   - 冷淡学长
badao_shaoye         - 霸道少爷
tianxin_xiaoling     - 甜心小玲
qiaopi_mengmei       - 俏皮萌妹
wumei_yujie          - 妩媚御姐
diadia_xuemei        - 嗲嗲学妹
danya_xuejie         - 淡雅学姐
Chinese (Mandarin)_Reliable_Executive    - 沉稳高管
Chinese (Mandarin)_News_Anchor           - 新闻女声
Chinese (Mandarin)_Mature_Woman          - 傲娇御姐
Chinese (Mandarin)_Unrestrained_Young_Man - 不羁青年
Arrogant_Miss        - 嚣张小姐
Robot_Armor          - 机械战甲
Chinese (Mandarin)_Kind-hearted_Antie   - 热心大婶
Chinese (Mandarin)_HK_Flight_Attendant  - 港普空姐
Chinese (Mandarin)_Humorous_Elder       - 搞笑大爷
Chinese (Mandarin)_Gentleman            - 温润男声
Chinese (Mandarin)_Warm_Bestie          - 温暖闺蜜
Chinese (Mandarin)_Male_Announcer       - 播报男声
Chinese (Mandarin)_Sweet_Lady           - 甜美女声
Chinese (Mandarin)_Southern_Young_Man   - 南方小哥
Chinese (Mandarin)_Wise_Women          - 阅历姐姐
Chinese (Mandarin)_Gentle_Youth         - 温润青年
Chinese (Mandarin)_Warm_Girl           - 温暖少女
Chinese (Mandarin)_Kind-hearted_Elder   - 花甲奶奶
Chinese (Mandarin)_Cute_Spirit         - 憨憨萌兽
Chinese (Mandarin)_Radio_Host          - 电台男主播
Chinese (Mandarin)_Lyrical_Voice       - 抒情男声
Chinese (Mandarin)_Straightforward_Boy - 率真弟弟
Chinese (Mandarin)_Sincere_Adult       - 真诚青年
Chinese (Mandarin)_Gentle_Senior       - 温柔学姐
Chinese (Mandarin)_Stubborn_Friend     - 嘴硬竹马
Chinese (Mandarin)_Crisp_Girl          - 清脆少女
Chinese (Mandarin)_Pure-hearted_Boy    - 清澈邻家弟弟
Chinese (Mandarin)_Soft_Girl           - 柔和少女
```
