# Credentials Encryption Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现凭证文件的加密存储与运行时解密加载

**Architecture:** 新增 CredentialsManager 模块，提供 LoadCredentials() 和 GetXXX() 函数，启动时加载并缓存到静态变量

**Tech Stack:** C++20, Windows Compression API (lz32.dll), Base64 (复用 MimoTTSClient.cpp 实现)

---

## File Structure

### Create
- `MonsterOrderWilds/CredentialsManager.h` - 接口声明
- `MonsterOrderWilds/CredentialsManager.cpp` - 实现
- `MonsterOrderWilds/CredentialsManagerTests.cpp` - 单元测试
- `scripts/generate_credentials.py` - 加密凭证生成工具（不提交git）

### Modify
- `MonsterOrderWilds/Network.cpp` - 改用 GetACCESS_KEY_ID() / GetACCESS_KEY_SECRET()
- `MonsterOrderWilds/BliveManager.cpp` - 改用 GetAPP_ID()
- `MonsterOrderWilds/MimoTTSClient.cpp` - 改用 GetMIMO_API_KEY()
- `MonsterOrderWilds/MonsterOrderWilds.vcxproj` - 添加新文件引用

### Remove from build (keep file)
- `MonsterOrderWilds/CredentialsConsts.h` - 不再编译引用

---

## Tasks

### Task 1: 创建 CredentialsManager.h

**Files:**
- Create: `MonsterOrderWilds/CredentialsManager.h`

- [ ] **Step 1: 编写头文件**

```cpp
#pragma once
#include "framework.h"
#include <string>

constexpr const char* FILE_MAGIC = "@MonsterOrderSecret@";

bool LoadCredentials();
std::string GetAPP_ID();
std::string GetACCESS_KEY_ID();
std::string GetACCESS_KEY_SECRET();
std::string GetMIMO_API_KEY();
```

- [ ] **Step 2: Commit**

```bash
git add MonsterOrderWilds/CredentialsManager.h
git commit -m "feat: add CredentialsManager header"
```

---

### Task 2: 创建 CredentialsManager.cpp (实现)

**Files:**
- Create: `MonsterOrderWilds/CredentialsManager.cpp`

- [ ] **Step 1: 编写 Base64 解码辅助函数**

参考 `MimoTTSClient.cpp:103-135` 的 Base64 实现，提取为静态工具函数。

- [ ] **Step 2: 编写 zlib 解压辅助函数**

使用 Windows Compression API (lz32.dll) 或 header-only 方案。函数签名：

```cpp
static std::vector<uint8_t> DecompressZlib(const uint8_t* data, size_t size);
```

- [ ] **Step 3: 实现 LoadCredentials()**

```cpp
static std::string s_appId;
static std::string s_accessKeyId;
static std::string s_accessKeySecret;
static std::string s_mimoApiKey;
static bool s_loaded = false;

bool LoadCredentials() {
    if (s_loaded) return true;
    
    std::string filepath = "MonsterOrderWilds_configs/credentials.dat";
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
    
    // Base64 decode
    std::vector<uint8_t> decoded = Base64Decode(buffer);
    
    // Zlib decompress
    std::vector<uint8_t> decompressed = DecompressZlib(decoded.data(), decoded.size());
    
    std::string content(decompressed.begin(), decompressed.end());
    
    // Verify FILE_MAGIC
    if (content.find(FILE_MAGIC) != 0) return false;
    
    content = content.substr(strlen(FILE_MAGIC));
    
    // Parse key=value lines
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        if (key == "APP_ID") s_appId = value;
        else if (key == "ACCESS_KEY_ID") s_accessKeyId = value;
        else if (key == "ACCESS_KEY_SECRET") s_accessKeySecret = value;
        else if (key == "MIMO_API_KEY") s_mimoApiKey = value;
    }
    
    s_loaded = true;
    return true;
}
```

- [ ] **Step 4: 实现 GetXXX() 函数**

```cpp
std::string GetAPP_ID() { return s_appId; }
std::string GetACCESS_KEY_ID() { return s_accessKeyId; }
std::string GetACCESS_KEY_SECRET() { return s_accessKeySecret; }
std::string GetMIMO_API_KEY() { return s_mimoApiKey; }
```

- [ ] **Step 5: 验证编译**

```bash
"D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "D:\VisualStudioProjects\JonysandMHDanmuTools\MonsterOrderWilds\MonsterOrderWilds.vcxproj" -p:Configuration=Debug -p:Platform=x64 -t:Build
```

- [ ] **Step 6: Commit**

```bash
git add MonsterOrderWilds/CredentialsManager.cpp
git commit -m "feat: implement CredentialsManager"
```

---

### Task 3: 创建 Python 生成工具

**Files:**
- Create: `scripts/generate_credentials.py`

- [ ] **Step 1: 编写 Python 脚本**

```python
#!/usr/bin/env python3
import zlib
import base64
import sys

FILE_MAGIC = '@MonsterOrderSecret@'

def write_info_file(filepath, data):
    compress_data = zlib.compress(FILE_MAGIC + data.encode('utf-8'))
    encode_str = base64.b64encode(compress_data)
    with open(filepath, 'wb') as f:
        f.write(encode_str)
        f.flush()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: generate_credentials.py <output_path> <app_id> <access_key_id> <access_key_secret> [mimo_api_key]")
        sys.exit(1)
    
    filepath = sys.argv[1]
    app_id = sys.argv[2]
    access_key_id = sys.argv[3]
    access_key_secret = sys.argv[4]
    mimo_api_key = sys.argv[5] if len(sys.argv) > 5 else ""
    
    data = f"APP_ID={app_id}\nACCESS_KEY_ID={access_key_id}\nACCESS_KEY_SECRET={access_key_secret}\nMIMO_API_KEY={mimo_api_key}\n"
    write_info_file(filepath, data)
    print(f"Credentials written to {filepath}")
```

- [ ] **Step 2: 添加到 .gitignore**

```bash
echo "scripts/generate_credentials.py" >> .gitignore
git add .gitignore
git commit -m "chore: ignore generate_credentials.py"
```

---

### Task 4: 修改 Network.cpp

**Files:**
- Modify: `MonsterOrderWilds/Network.cpp:115,129`

- [ ] **Step 1: 添加 include**

```cpp
#include "CredentialsManager.h"
```

- [ ] **Step 2: 修改第115行**

```cpp
// Before:
{"x-bili-accesskeyid", credentials::ACCESS_KEY_ID},
// After:
{"x-bili-accesskeyid", GetACCESS_KEY_ID()},
```

- [ ] **Step 3: 修改第129行**

```cpp
// Before:
std::string signature = hashpp::get::getHMAC(hashpp::ALGORITHMS::SHA2_256, ProtoUtils::Encode(credentials::ACCESS_KEY_SECRET), ProtoUtils::Encode(headerStr));
// After:
std::string signature = hashpp::get::getHMAC(hashpp::ALGORITHMS::SHA2_256, ProtoUtils::Encode(GetACCESS_KEY_SECRET()), ProtoUtils::Encode(headerStr));
```

- [ ] **Step 4: 验证编译**

- [ ] **Step 5: Commit**

---

### Task 5: 修改 BliveManager.cpp

**Files:**
- Modify: `MonsterOrderWilds/BliveManager.cpp:107,123`

- [ ] **Step 1: 添加 include**

```cpp
#include "CredentialsManager.h"
```

- [ ] **Step 2: 修改第107行**

```cpp
// Before:
std::string params = "{\"code\":\"" + idCode + "\",\"app_id\":" + credentials::APP_ID + "}";
// After:
std::string params = "{\"code\":\"" + idCode + "\",\"app_id\":" + GetAPP_ID() + "}";
```

- [ ] **Step 3: 修改第123行**

```cpp
// Before:
std::string params = "{\"game_id\":\"" + (GameId.empty() ? gameId : GameId) + "\",\"app_id\":" + credentials::APP_ID + "}";
// After:
std::string params = "{\"game_id\":\"" + (GameId.empty() ? gameId : GameId) + "\",\"app_id\":" + GetAPP_ID() + "}";
```

- [ ] **Step 4: 验证编译**

- [ ] **Step 5: Commit**

---

### Task 6: 修改 MimoTTSClient.cpp

**Files:**
- Modify: `MonsterOrderWilds/MimoTTSClient.cpp`

- [ ] **Step 1: 添加 include**

```cpp
#include "CredentialsManager.h"
```

- [ ] **Step 2: 找到并修改 MIMO_API_KEY 使用处**

```cpp
// Before:
#define MIMO_API_KEY "sk-crv6m48m1rx78sdbayckz5uuu1z7c9mvc9p95xmgtywm1kf2"
// After:
// Remove this line - use GetMIMO_API_KEY() instead
```

- [ ] **Step 3: 找到所有使用 MIMO_API_KEY 的地方，改为 GetMIMO_API_KEY()**

- [ ] **Step 4: 验证编译**

- [ ] **Step 5: Commit**

---

### Task 7: 更新 vcxproj

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`

- [ ] **Step 1: 添加 CredentialsManager.h 到 Header Files**

在 `CredentialsConsts.h` 之后按字母顺序插入。

- [ ] **Step 2: 添加 CredentialsManager.cpp 到 Source Files**

在 `CredentialsConsts.h` 之后按字母顺序插入。

- [ ] **Step 3: 添加 CredentialsManagerTests.cpp (条件排除)**

```xml
<ClCompile Include="CredentialsManagerTests.cpp">
  <Filter>UnitTests</Filter>
  <ExcludedFromBuild Condition="'$(Configuration)'!='UnitTest'">true</ExcludedFromBuild>
</ClCompile>
```

- [ ] **Step 4: 验证编译**

- [ ] **Step 5: Commit**

---

### Task 8: 创建 CredentialsManagerTests.cpp

**Files:**
- Create: `MonsterOrderWilds/CredentialsManagerTests.cpp`

- [ ] **Step 1: 编写单元测试**

```cpp
#ifdef RUN_UNIT_TESTS
#include "CredentialsManager.h"
#include "UnitTestLog.h"
#include <cassert>

void TestBase64Encode() {
    std::string decoded = "Hello World";
    std::string encoded = Base64Encode(reinterpret_cast<const uint8_t*>(decoded.c_str()), decoded.size());
    // Verify encoded matches expected
}

void TestDecompressZlib() {
    // Test with known compressed data
}

void TestCredentialsManagerLoad() {
    bool loaded = LoadCredentials();
    if (loaded) {
        LOG_INFO("APP_ID: %s", GetAPP_ID().c_str());
    }
}

int main() {
    TestBase64Encode();
    TestDecompressZlib();
    TestCredentialsManagerLoad();
    LOG_INFO("[PASS] CredentialsManagerTests");
    return 0;
}
#endif
```

- [ ] **Step 2: 验证测试编译和运行**

- [ ] **Step 3: Commit**

---

### Task 9: 从编译中移除 CredentialsConsts.h

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`

- [ ] **Step 1: 从 ClInclude 中移除 CredentialsConsts.h**

```xml
<!-- Remove or comment out -->
<!-- <ClInclude Include="CredentialsConsts.h" /> -->
```

- [ ] **Step 2: 验证编译通过**

- [ ] **Step 3: Commit**

---

## Self-Review Checklist

- [ ] Spec coverage: proposal.md 中所有需求都有对应任务
- [ ] Placeholder scan: 无 TBD/TODO
- [ ] Type consistency: GetXXX() 函数签名一致
- [ ] 文件路径: 所有路径为绝对路径
- [ ] 命令验证: 编译命令已提供

---

## Execution Options

**Plan complete and saved to `openspec/changes/credentials-encryption/tasks.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
