## Context

当前 TTSManager 直接持有 `MimoTTSClient* mimoClient`，违反依赖倒置原则。MiMo 特有概念（dialect/role）残留在 AsyncTTSRequest 结构体中。项目中已存在 `ITTSProvider` 接口但未被 TTSManager 使用。

## Goals / Non-Goals

**Goals:**
- TTSManager 通过 `std::unique_ptr<ITTSProvider>` 接口调用 TTS 提供者
- 删除 AsyncTTSRequest 中 dialect、role 残留字段
- 消除直接调用 `mimoClient` 的代码路径

**Non-Goals:**
- 不修改 ITTSProvider 接口现有方法签名
- 不改变 TTSManager 的公开接口（公开方法不变）
- 不重构 TTS 异步状态机逻辑

## Decisions

### Decision 1: 使用 std::unique_ptr<ITTSProvider> 持有提供者

**选择：** `std::unique_ptr<ITTSProvider> ttsProvider`

**理由：**
- 清晰表达独占所有权语义
- 自动管理生命周期，避免内存泄漏
- 接口不支持克隆，unique_ptr 最合适

### Decision 2: 删除 MimoTTSClient

**选择：** 删除 `MimoTTSClient` 类

**理由：**
- XiaomiTTSProvider 与 MimoTTSClient 功能完全重复（相同 API endpoint，相同响应格式）
- XiaomiTTSProvider 已实现 ITTSProvider 接口，可直接使用
- 删除冗余代码，减少维护成本

### Decision 3: 删除 AsyncTTSRequest 中的残留字段

**选择：** 从 AsyncTTSRequest 删除 dialect、role 字段

**理由：**
- 配置已删除，字段已无意义
- 减少内存占用
- 保持结构体整洁

## Architecture

```
重构前：
TTSManager -> MimoTTSClient (直接调用)

重构后：
TTSManager -> unique_ptr<ITTSProvider> -> XiaomiTTSProvider
                                       -> SapiTTSProvider
                                       -> MiniMaxTTSProvider
```

## Risks / Trade-offs

**风险：**
- 重构过程中可能引入回归问题
- 依赖注入改变可能影响异常安全

**权衡：**
- 通过接口解耦带来的可测试性提升 > 短期重构成本
- 已有单元测试覆盖可降低回归风险
