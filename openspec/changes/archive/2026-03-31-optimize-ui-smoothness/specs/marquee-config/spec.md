# 跑马灯默认文本配置

## 1. 概述

**功能描述**：在设置界面的"其他设置"标签页中添加一个文本输入框，允许用户自定义跑马灯的默认显示内容。

**用户价值**：用户可以根据自己的喜好自定义跑马灯的默认提示文字，提升个性化体验。

## 2. 配置项详情

| 属性 | 值 |
|------|-----|
| 配置键名 | `DEFAULT_MARQUEE_TEXT` (C#) / `defaultMarqueeText` (C++) |
| 数据类型 | string |
| 默认值 | `发送'点怪 xxx'进行点怪` |
| 持久化 | 是，存储在 `MainConfig.cfg` |

## 3. UI 规范

### 3.1 布局位置

- **位置**："其他设置" 标签页最开头
- **顺序**：在"点怪窗口透明度"设置之前

### 3.2 UI 组件

```
┌─────────────────────────────────────────────────┐
│ 跑马灯默认文本                                    │
│ ┌─────────────────────────────────────────────┐ │
│ │ 发送'点怪 xxx'进行点怪                        │ │
│ └─────────────────────────────────────────────┘ │
│                                                 │
│ 点怪窗口透明度                                   │
│ [──────●──────] 80                             │
└─────────────────────────────────────────────────┘
```

### 3.3 组件规格

| 属性 | 值 |
|------|-----|
| Label | "跑马灯默认文本" |
| TextBox Width | 300 |
| TextBox Height | 24 |
| Margin | 8,8,0,0 |

## 4. 数据流

### 4.1 配置变更流程

```
用户输入 → TextBox.TextChanged 
→ ConfigWindow.DefaultMarqueeTextTextBox_TextChanged
→ GlobalEventListener.Invoke("ConfigChanged", "DEFAULT_MARQUEE_TEXT:xxx")
→ ToolsMain.ConfigChanged
→ MainConfig.DEFAULT_MARQUEE_TEXT
→ ConfigFieldRegistry.Set("defaultMarqueeText", value)
→ C++ ConfigData.defaultMarqueeText
→ ConfigManager.MarkDirty()
→ 下次 SaveConfig 时持久化到 MainConfig.cfg
```

### 4.2 配置读取流程

```
程序启动 → ConfigManager.LoadConfig()
→ 从 MainConfig.cfg 读取 defaultMarqueeText
→ OrderedMonsterWindow 构造函数
→ ToolsMain.GetConfigService().Config.DEFAULT_MARQUEE_TEXT
→ 设置 _defaultInfo 占位符
```

## 5. 实施任务

| 任务 | 描述 | 状态 |
|------|------|------|
| 5.1 | C++ ConfigManager.h: ConfigData 添加 defaultMarqueeText 字段 | 待实施 |
| 5.2 | C++ ConfigFieldRegistry.cpp: 注册 defaultMarqueeText 字段 | 待实施 |
| 5.3 | C++ ConfigManager.cpp: JSON 加载/保存处理 | 待实施 |
| 5.4 | C++/CLI DataBridgeWrapper.h: ConfigProxy 添加属性 | 待实施 |
| 5.5 | C# DataStructures.cs: ConfigDataSnapshot 添加字段 | 待实施 |
| 5.6 | C# Utils.cs: MainConfig 添加 DEFAULT_MARQUEE_TEXT 属性 | 待实施 |
| 5.7 | C# ProxyClasses.cs: ConfigProxy 添加属性 | 待实施 |
| 5.8 | C# ToolsMain.cs: ConfigChanged 处理 DEFAULT_MARQUEE_TEXT | 待实施 |
| 5.9 | C# ConfigWindow.xaml: 添加 TextBox | 待实施 |
| 5.10 | C# ConfigWindow.xaml.cs: TextChanged 事件处理 | 待实施 |
| 5.11 | C# OrderedMonsterWindow.xaml.cs: 使用配置值替代硬编码常量 | 待实施 |

## 6. 技术细节

### 6.1 C++ ConfigData 结构变更

```cpp
struct ConfigData
{
    // ... 已有字段 ...
    
    // 跑马灯默认文本
    std::string defaultMarqueeText = "发送'点怪 xxx'进行点怪";
};
```

### 6.2 JSON 配置格式

```json
{
    "DEFAULT_MARQUEE_TEXT": "发送'点怪 xxx'进行点怪",
    // ... 其他配置 ...
}
```

### 6.3 字段注册

```cpp
REGISTER_FIELD("defaultMarqueeText", std::string, defaultMarqueeText, ConfigFieldType::String);
```

## 7. 兼容性

- **向后兼容**：如果配置文件中没有 `DEFAULT_MARQUEE_TEXT`，使用代码中的默认值
- **迁移**：现有用户的配置文件中不会自动添加此字段，首次保存时会自动添加

## 8. 验证记录

| 日期 | 验证项 | 结果 |
|------|--------|------|
| 2026-03-31 | 设置界面显示默认文本输入框 | ✅ 通过 |
| 2026-03-31 | 修改默认文本后跑马灯立即更新 | ✅ 通过 |
| 2026-03-31 | 重启后默认文本配置保持 | ✅ 通过 |

**注意**: `MonsterOrderWilds_configs` 目录必须与 `MonsterOrderWilds.exe` 在同一目录下。