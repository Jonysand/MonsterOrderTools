# DeepSeek Chat AI Provider - Tasks

## Task 1: 创建 DeepSeekAIChatProvider.h

**Files:**
- Create: `MonsterOrderWilds/DeepSeekAIChatProvider.h`

**Steps:**
- [ ] 创建头文件，继承 `IAIChatProvider`
- [ ] 声明构造函数 `DeepSeekAIChatProvider(const std::string& apiKey)`
- [ ] 声明 `GetProviderName()`, `IsAvailable()`, `GetLastError()`, `CallAPI()`
- [ ] 声明私有方法 `MakeSyncHttpsRequest()`
- [ ] 声明私有成员: `apiKey_`, `lastError_`, `available_`

---

## Task 2: 创建 DeepSeekAIChatProvider.cpp

**Files:**
- Create: `MonsterOrderWilds/DeepSeekAIChatProvider.cpp`

**Steps:**
- [ ] 实现构造函数，初始化 `available_ = false`
- [ ] 实现 `GetProviderName()` 返回 `"deepseek"`
- [ ] 实现 `CallAPI()`:
  - 构建 JSON 请求体: `{"model":"deepseek-chat","messages":[{"role":"user","content":"{prompt}"}]}`
  - 调用 `MakeSyncHttpsRequest("api.deepseek.com", 443, "/chat/completions", headers, body, outResponse)`
  - 解析响应: `outResponse = j["choices"][0]["message"]["content"]`
  - 成功时设置 `available_ = true`
  - 失败时设置 `lastError_`
- [ ] 复制 `MakeSyncHttpsRequest()` 实现（参考 `MiniMaxAIChatProvider.cpp`）

**Verification:**
- [ ] 编译: `MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`

---

## Task 3: 更新 AIChatProviderFactory.cpp

**Files:**
- Modify: `MonsterOrderWilds/AIChatProviderFactory.cpp`

**Steps:**
- [ ] 添加 `#include "DeepSeekAIChatProvider.h"`
- [ ] 在 `Create()` 函数中添加分支:
  ```cpp
  if (provider == "deepseek") {
      return std::make_unique<DeepSeekAIChatProvider>(apiKey);
  }
  ```

**Verification:**
- [ ] 编译: `MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`

---

## Task 4: 创建 DeepSeekAIChatProviderTests.cpp

**Files:**
- Create: `MonsterOrderWilds/DeepSeekAIChatProviderTests.cpp`

**Steps:**
- [ ] 编写 `TestDeepSeekAIChatProvider_Interface`:
  ```cpp
  static_assert(std::is_base_of<IAIChatProvider, DeepSeekAIChatProvider>::value,
      "DeepSeekAIChatProvider must inherit IAIChatProvider");
  ```
- [ ] 编写 `TestDeepSeekAIChatProvider_ProviderName`:
  ```cpp
  auto provider = std::make_unique<DeepSeekAIChatProvider>("test_key");
  assert(provider->GetProviderName() == "deepseek");
  ```
- [ ] 编写 `TestDeepSeekAIChatProviderFactory_Create`:
  ```cpp
  auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"deepseek","chat_api_key":"test_key"})");
  assert(provider != nullptr);
  assert(provider->GetProviderName() == "deepseek");
  ```
- [ ] 编写 `TestDeepSeekAIChatProviderFactory_UnsupportedProvider`:
  ```cpp
  auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"unknown","chat_api_key":"test_key"})");
  assert(provider == nullptr);
  ```
- [ ] 编写 `TestDeepSeekAIChatProviderFactory_EmptyApiKey`:
  ```cpp
  auto provider = AIChatProviderFactory::Create(R"({"chat_provider":"deepseek","chat_api_key":""})");
  assert(provider == nullptr);
  ```
- [ ] 添加 `RunDeepSeekAIChatProviderTests()` 函数
- [ ] 在 `UnitTestMain.cpp` 中调用 `RunDeepSeekAIChatProviderTests()`

**Verification:**
- [ ] 编译: `MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=UnitTest -p:Platform=x64 -t:Build`

---

## Task 5: 更新项目文件

**Files:**
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj`
- Modify: `MonsterOrderWilds/MonsterOrderWilds.vcxproj.filters`
- Modify: `MonsterOrderWilds/BliveManagerTests.vcxproj.filters`

**Steps:**
- [ ] 在 `MonsterOrderWilds.vcxproj` 的 `ClInclude` 中添加 `DeepSeekAIChatProvider.h`
- [ ] 在 `MonsterOrderWilds.vcxproj` 的 `ClCompile` 中添加 `DeepSeekAIChatProvider.cpp`
- [ ] 添加 `DeepSeekAIChatProviderTests.cpp`（条件排除 Release）
- [ ] 在 `.vcxproj.filters` 中添加对应 Filter 节点

**Filter 映射:**
| 文件 | Filter |
|------|--------|
| `DeepSeekAIChatProvider.h` | DataProcessing |
| `DeepSeekAIChatProvider.cpp` | DataProcessing |
| `DeepSeekAIChatProviderTests.cpp` | UnitTests |

**Verification:**
- [ ] 编译: `MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] Git diff 检查修改范围

---

## Task 6: 编译验证

**Steps:**
- [ ] Debug 编译: `MSBuild.exe MonsterOrderWilds.sln -p:Configuration=Debug -p:Platform=x64 -t:Build`
- [ ] UnitTest 编译: `MSBuild.exe MonsterOrderWilds.vcxproj -p:Configuration=UnitTest -p:Platform=x64 -t:Build`
- [ ] 运行单元测试，验证所有 `[PASS]`