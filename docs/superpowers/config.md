# 项目配置

## 项目信息

- **名称**: JonysandMHDanmuTools
- **描述**: B站弹幕点怪工具，允许用户通过发送弹幕预约特定怪物
- **技术栈**: C++ (核心业务逻辑) + C# (UI)
- **平台**: Windows

## 技术架构

### 目录结构

- `MonsterOrderWilds/`: C++ 核心层（网络、弹幕处理、数据管理、TTS）
- `JonysandMHDanmuTools/`: C# UI 层（WPF）
- 通信：C++/CLI Bridge via DataBridgeWrapper

### 核心模块

| 模块 | 职责 |
|------|------|
| BliveManager | B站连接生命周期管理（连接/断开/重连） |
| DanmuProcessor | 弹幕消息解析和处理 |
| ConfigManager | 配置持久化（JSON） |
| TTSManager | MiMo TTS 语音合成 |
| AudioPlayer | Windows MCI 音频播放 |
| StringProcessor | UTF-8/中文字符串处理 |
| CredentialsManager | 敏感信息加密存储 |
| EventSystem | C++ 事件分发（单播/多播委托） |
| ErrorHandler | 统一错误处理和报告 |
| WriteLog | 日志系统 |
| DumpHelper | 崩溃转储 |

## 命名规范

### 配置字段命名

| 层 | 命名风格 | 示例 |
|----|---------|------|
| C++ ConfigData | camelCase | `defaultMarqueeText` |
| JSON 配置 | SCREAMING_SNAKE | `DEFAULT_MARQUEE_TEXT` |
| C# MainConfig | SCREAMING_SNAKE | `DEFAULT_MARQUEE_TEXT` |
| UI 控件 Name | PascalCase | `DefaultMarqueeTextBox` |

### 文件编码

- 源文件编码: UTF-8 with BOM
- vcxproj 文件组织: 字母顺序排列

## 文档规范

### Spec 规范

- 每个 spec 应对应一个明确的功能模块
- 保持 specs 目录整洁，一个模块一个 spec.md
- spec 文件使用 markdown 格式

### Change 规范

- 不要删除已归档的 change
- 新 change 放在 changes/ 目录下
- 归档的 change 放在 changes/archive/ 下

### Tasks 规范

- 任务描述要具体可执行
- 避免超过 2 小时的复杂任务，必要时拆分

### Proposal 规范

- 提案应简洁，核心内容不超过 500 字
- 必须包含 Non-goals 章节明确范围外内容