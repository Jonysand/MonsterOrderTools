## 0. 单元测试

> **实施优先级**: 优先编写，确保原有功能正常实施

- [x] 0.1 创建 ConfigManagerRegistryTests.cpp 测试文件
- [x] 0.2 编写注册表读取测试（无值时返回空字符串）
- [x] 0.3 编写注册表写入测试
- [x] 0.4 编写注册表读写集成测试
- [x] 0.5 运行单元测试验证所有测试通过

**测试文件**: `MonsterOrderWilds/ConfigManagerRegistryTests.cpp`

**测试命令**: `MSBuild.exe "JonysandMHDanmuTools.sln" -p:Configuration=UnitTest -p:Platform=x64 -t:Build -m`

## 1. 注册表读写实现

- [x] 1.1 在 ConfigManager.cpp 中添加注册表操作辅助函数
  - `ReadIdCodeFromRegistry()` - 从注册表读取 idCode
  - `WriteIdCodeToRegistry()` - 写入 idCode 到注册表

**注册表路径**: `HKEY_CURRENT_USER\Software\MonsterOrderWilds`
**键名**: `IdCode` (REG_SZ 类型)

- [x] 1.2 修改 `LoadConfig()` 方法
  - 从 JSON 加载除 idCode 外的所有字段
  - 从注册表读取 idCode

- [x] 1.3 修改 `SaveConfig()` 方法
  - 保存除 idCode 外的所有字段到 JSON
  - 将 idCode 写入注册表

**实现方案**:

```cpp
// 注册表路径
const char* REG_SUBKEY = "Software\\MonsterOrderWilds";
const char* REG_VALUE_NAME = "IdCode";

// 从注册表读取
std::string ReadIdCodeFromRegistry() {
    HKEY hKey;
    std::string result = "";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, REG_VALUE_NAME, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            result = buffer;
        }
        RegCloseKey(hKey);
    }
    return result;
}

// 写入注册表
bool WriteIdCodeToRegistry(const std::string& idCode) {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, REG_VALUE_NAME, 0, REG_SZ, (const BYTE*)idCode.c_str(), idCode.length() + 1);
        RegCloseKey(hKey);
        return true;
    }
    return false;
}
```

## 2. 编译验证

- [x] 2.1 使用 MSBuild 编译 Release 配置
- [x] 2.2 确认编译无错误无警告

**编译命令**: `MSBuild.exe "JonysandMHDanmuTools.sln" -p:Configuration=Release -p:Platform=x64 -t:Build -m`

**编译结果**: 成功生成，警告为已存在的 C4477 格式字符串问题

## 3. 功能验证

- [ ] 3.1 修改 idCode 后关闭程序，重新打开 idCode 保持
- [ ] 3.2 注册表中可看到 IdCode 值
- [ ] 3.3 JSON 配置文件中 ID_CODE 字段不再更新（保持原值作为备份）

## 决策记录

| 决策点 | 选择 | 理由 |
|--------|------|------|
| 注册表路径 | HKEY_CURRENT_USER | 用户级隔离，不需要管理员权限 |
| 注册表键名 | IdCode | 与C++字段名一致 |
| 首次无值处理 | 返回空字符串 | 不影响程序启动，用户需重新输入 |
| 旧JSON数据 | 保留不删除 | 向后兼容，数据迁移需要 |
