# Manbo TTS 修复 - Tasks

## 任务列表

- [x] 修复 TTSProviderFactory 缺少 manbo 和 minimax 处理
- [x] 修复 Manbo API URL 查询参数处理
- [x] 添加 TTSResponse format 字段
- [x] 修复 AudioPlayer 临时文件创建（查询参数和默认值）
- [x] 编译验证

## 修复内容

| 文件 | 修改 |
|------|------|
| `TTSProviderFactory.cpp` | 添加 manbo 和 minimax 显式处理 |
| `ManboTTSProvider.cpp` | 正确提取 URL 扩展名 |
| `ITTSProvider.h` | TTSResponse 添加 format 字段 |
| `AudioPlayer.cpp` | 处理查询参数，默认 mp3 |
| `TextToSpeech.cpp` | 使用 response.format |