## Why

idCode（身份码）用于用户身份验证，需要持久化保存。当前实现将 idCode 存储在 JSON 配置文件中，存在以下问题：
- 配置文件与可执行文件耦合，需要保持相对路径
- 配置文件可能被误删或移动
- 多实例运行时可能产生冲突

改用 Windows 注册表存储 idCode：
- 持久化存储，不依赖文件路径
- 用户级隔离，不影响其他用户
- Windows 系统级管理，更可靠

## What Changes

1. 修改 `ConfigManager::LoadConfig()` - idCode 从注册表读取，不再从 JSON 读取
2. 修改 `ConfigManager::SaveConfig()` - idCode 写入注册表，不再写入 JSON
3. JSON 配置文件中的 `ID_CODE` 字段保留但不再使用（向后兼容）

## Capabilities

### Modified Capabilities
- `config-persistence`: idCode 持久化方式从 JSON 文件改为 Windows 注册表

## Impact

- 修改 `MonsterOrderWilds/ConfigManager.cpp` 的 `LoadConfig()` 和 `SaveConfig()` 方法
- 注册表路径：`HKEY_CURRENT_USER\Software\MonsterOrderWilds\IdCode`
- JSON 配置文件中 `ID_CODE` 字段保留，不影响旧版本数据迁移
