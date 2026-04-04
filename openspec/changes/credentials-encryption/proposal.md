# Credentials Encryption Design

## Goal

将 `CredentialsConsts.h` 中的明文凭证改为加密存储，启动时从 `MonsterOrderWilds_configs/credentials.dat` 解密加载。

## Architecture

### 新增文件

| 文件 | 职责 |
|------|------|
| `MonsterOrderWilds/CredentialsManager.h` | 接口声明 |
| `MonsterOrderWilds/CredentialsManager.cpp` | 实现：Base64解码 + zlib解压 + 字段解析 + 静态缓存 |
| `MonsterOrderWilds/CredentialsManagerTests.cpp` | 单元测试 |

### 接口

```cpp
bool LoadCredentials();
std::string GetAPP_ID();
std::string GetACCESS_KEY_ID();
std::string GetACCESS_KEY_SECRET();
std::string GetMIMO_API_KEY();
```

### 静态缓存

- 首次调用 `LoadCredentials()` 时加载并缓存到静态变量
- `GetXXX()` 函数返回缓存值，无锁线程安全（单线程初始化场景）

## File Format

```
[base64(zlib('@MonsterOrderSecret@' + 'APP_ID=xxx\nACCESS_KEY_ID=xxx\nACCESS_KEY_SECRET=xxx\nMIMO_API_KEY=xxx\n'))]
```

## Loading Flow

1. `LoadCredentials()` 读取 `MonsterOrderWilds_configs/credentials.dat`
2. Base64解码 → zlib解压
3. 验证 `FILE_MAGIC = "@MonsterOrderSecret@"`
4. 按行解析 `key=value` 格式
5. 缓存到静态变量

## Error Handling

- 文件不存在：记录日志，返回 false
- 格式错误（无FILE_MAGIC）：返回空字符串，不崩溃
- 字段缺失：对应 `GetXXX()` 返回空字符串

## Changes to Existing Files

| 文件 | 修改内容 |
|------|---------|
| `Network.cpp` | `credentials::ACCESS_KEY_ID` → `GetACCESS_KEY_ID()`，`credentials::ACCESS_KEY_SECRET` → `GetACCESS_KEY_SECRET()` |
| `BliveManager.cpp` | `credentials::APP_ID` → `GetAPP_ID()` |
| `MimoTTSClient.cpp` | `MIMO_API_KEY` → `GetMIMO_API_KEY()` |

## Dependencies

- zlib：使用 Windows 内置或项目已有库
- Base64：参考 `MimoTTSClient.cpp` 中已有实现

## Removed Files

- `CredentialsConsts.h`：不再需要，凭证从文件加载

## Testing

- 单元测试：验证加密文件生成、解析、错误处理
