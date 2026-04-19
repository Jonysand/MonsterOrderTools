# Manbo TTS 修复 - Design Document

## 概述

本次修复解决了 Manbo TTS 引擎无法正常播放的问题。

## Bug 修复详情

### Bug 1: TTSProviderFactory 缺少 manbo 和 minimax 处理

**问题描述:** 在 UI 切换 TTS engine 到 manbo 或 minimax 时，设置不生效。

**根因分析:**
- `TTSProviderFactory::Create()` 只处理 "sapi" 和 "mimo"
- 选择 "manbo" 或 "minimax" 时走到 AUTO 模式，用户选择的引擎不生效

**修复方案:**
- 添加 "minimax" 和 "manbo" 的显式处理分支
- AUTO 模式改为 manbo 优先（运行时通过 `TrySwitchToNextProvider` 降级）

**涉及文件:**
- `TTSProviderFactory.cpp` - 添加 manbo 和 minimax 分支

---

### Bug 2: Manbo API URL 查询参数未正确处理

**问题描述:** Manbo TTS API 返回的 audio_url 包含 AWS S3 签名查询参数（如 `?x-amz-algorithm=...`），导致：
1. 提取扩展名时包含查询参数
2. `AudioPlayer::WriteToTempFile` 创建临时文件失败（Windows 错误 3: ERROR_PATH_NOT_FOUND）

**根因分析:**
- `ManboTTSProvider::DownloadAudio` 从 URL 提取扩展名时未处理查询参数
- 例如：`audio.wav?x-amz-algorithm=...` 被提取为 `wav?x-amz-algorithm=...`

**修复方案:**
1. `ManboTTSProvider::DownloadAudio` - 从 URL 提取扩展名时正确处理查询参数
2. `AudioPlayer::WriteToTempFile` - 添加防御性代码，处理 format 中的查询参数
3. `AudioPlayer::WriteToTempFile` - format 为空时默认使用 .mp3

**涉及文件:**
- `ManboTTSProvider.cpp` - 正确提取 URL 扩展名
- `AudioPlayer.cpp` - 处理查询参数和默认值

---

### Bug 3: TTSResponse 缺少 format 字段

**问题描述:** Manbo TTS 返回的音频格式不确定，但其他 TTS 提供商硬编码使用 "mp3"。

**根因分析:**
- 不同 TTS API 返回的音频格式可能不同（mp3/wav/m4a 等）
- `TTSResponse` 没有存储格式信息的字段

**修复方案:**
- `ITTSProvider.h` - `TTSResponse` 结构体添加 `format` 字段
- `ManboTTSProvider::DownloadAudio` - 从 URL 提取并返回正确格式
- `TextToSpeech.cpp` - 使用 response.format 更新请求的 responseFormat

**涉及文件:**
- `ITTSProvider.h` - TTSResponse 添加 format 字段
- `ManboTTSProvider.cpp` - 设置 response.format
- `TextToSpeech.cpp` - 使用 response.format

---

## 验证方式

编译 Debug 版本后运行程序，测试：
1. UI 切换 TTS engine 到 manbo
2. 发送弹幕触发 TTS 播报
3. 确认日志显示临时文件创建成功：`AudioPlayer: Temp file saved to: ...`
4. 确认音频正常播放

## 编译验证

✅ MSBuild Debug x64 编译通过，0 个错误