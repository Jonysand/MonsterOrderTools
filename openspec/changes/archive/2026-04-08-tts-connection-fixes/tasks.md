# Tasks - 2026-04-08 TTS 和连接修复

## [DONE] 1. 修复 Network.cpp std::mutex 崩溃

- [x] 使用 shared_ptr/weak_ptr 模式管理 HttpsAsyncContext 生命周期
- [x] 分离 cleanupFlag 为独立的 shared_ptr
- [x] WinHttpSetStatusCallback 传入 NULL 禁用回调
- [x] 添加调试日志

**修改文件**: `MonsterOrderWilds/Network.cpp`

## [DONE] 2. 修复 WINHTTP_FLAG_ASYNC 错误 12019

- [x] 移除 WINHTTP_FLAG_ASYNC 标志
- [x] 使用同步 WinHTTP 模式

**修改文件**: `MonsterOrderWilds/Network.cpp`

## [DONE] 3. 修复 SAPI 音量默认值

- [x] ConfigManager.h 中 speechVolume 默认值 0 → 50
- [x] ConfigWindow.xaml 中滑块默认值 80 → 50
- [x] ProxyClasses.cs 中 _speechVolume 默认值 80 → 50

**修改文件**: 
- `MonsterOrderWilds/ConfigManager.h`
- `JonysandMHDanmuTools/ConfigWindow.xaml`
- `JonysandMHDanmuTools/ProxyClasses.cs`

## [DONE] 4. 统一 MiMo TTS API Key 字段

- [x] CredentialsManager.cpp 读取 tts_api_key 而非 MIMO_API_KEY

**修改文件**: `MonsterOrderWilds/CredentialsManager.cpp`

## [DONE] 5. 修复 GetAI_PROVIDER() JSON 构建

- [x] 自动从 tts_provider, tts_api_key, chat_provider, chat_api_key 构建 JSON
- [x] 包含 TTS 和 Chat 两种 Provider 配置

**修改文件**: `MonsterOrderWilds/CredentialsManager.cpp`

## 验证清单

- [x] 编译成功 (0 errors)
- [x] 连接功能正常工作
- [x] SAPI TTS 可以播放（音量 50）
- [x] credentials.dat 字段 tts_api_key 被正确读取
