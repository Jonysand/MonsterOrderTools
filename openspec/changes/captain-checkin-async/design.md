## Context

当前舰长签到模块 `CaptainCheckInModule::PushDanmuEvent()` 在持有 `profilesLock_` 锁的情况下执行以下操作：
1. 数据库读写（LoadProfileFromDb/SaveProfileToDb）
2. AI API 同步调用（GenerateCheckinAnswerSync）

高并发场景下（每秒 100+ 弹幕），UI 线程被阻塞 1-3 秒，导致应用无响应。

## Goals / Non-Goals

**Goals:**
- UI 线程不被签到处理阻塞，保持 60fps 响应
- 高并发场景下（每秒 100+ 弹幕）音频播放不丢失
- 数据库操作不影响弹幕处理主流程
- 保持现有功能逻辑不变（AI 回复内容、TTS 引擎选择）

**Non-Goals:**
- 不改变 AI 回复生成逻辑和 Prompt 模板
- 不改变用户画像数据结构和 SQLite Schema
- 不改变 TTS 引擎选择逻辑
- 不改变现有配置字段

## Decisions

### Decision 1: 锁粒度优化

**选择**: `PushDanmuEvent` 只在访问共享数据时短暂持锁，AI 调用在锁外执行，DB 操作（SaveProfileToDb）在锁内快速完成

**实际实现**: AI 调用（GenerateCheckinAnswerAsync）在锁外异步执行，数据库写入在锁内同步完成（SaveProfileToDb）。由于数据库操作本身很快（约几毫秒），且舰长签到是低频操作（每个舰长每天一次），当前锁粒度在高并发场景下不会成为瓶颈。

**理由**: 
- 原问题：`profilesLock_` 在 AI API 等待期间被持有，阻塞其他舰长弹幕处理
- 解决：将数据访问和业务逻辑分离。数据访问时短暂持锁，业务逻辑在锁外执行

**替代方案考虑**:
- 读写锁（shared_mutex）: C++17 才支持，且写操作仍会阻塞
- 锁分段: 需要重构数据结构，改动过大

### Decision 2: AI 调用异步化

**选择**: 新增 `GenerateCheckinAnswerAsync` 方法，使用 `std::thread` 执行 API 调用，结果通过回调返回

**理由**:
- AI API 调用是主要阻塞点（1-3 秒等待）
- 异步化后 UI 线程立即返回，继续处理其他弹幕
- 回调模式与现有 `AnswerCallback` 接口兼容

**实现**:
```cpp
void CaptainCheckInModule::GenerateCheckinAnswerAsync(
    const CheckinEvent& event, 
    AnswerCallback callback) {
    // 复制必要数据，避免引用失效
    CheckinEvent evtCopy = event;
    auto resultCb = [callback](const AnswerResult& result) {
        // 回调仍在后台线程，需要切换到主线程？
        if (callback) callback(result);
    };
    
    std::thread([this, evtCopy, resultCb]() {
        AnswerResult result = GenerateCheckinAnswerSync(evtCopy);
        resultCb(result);
    }).detach();
}
```

### Decision 3: 数据库操作异步化

**选择**: `LoadProfileFromDb` 和 `SaveProfileToDb` 在后台线程执行

**理由**:
- SQLite 操作虽然快（<10ms），但在锁内累积仍会造成瓶颈
- 异步化后可以将多次写操作合并，减少 I/O 次数

**替代方案考虑**:
- 批量写入: 需要额外逻辑管理批次，且改动较大
- 内存缓存 + 延迟写回: 可能丢失数据，风险高

### Decision 4: TTS 队列并发优化

**选择**: 保留 `asyncPendingQueue_` 串行队列，但增加并发请求上限

**理由**:
- 音频播放必须串行（避免混音）
- 但可以同时发起多个 TTS HTTP 请求
- 修改 `hasCurrentRequest_` 为计数器，支持多请求并发

## Risks / Trade-offs

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| 异步回调在后台线程执行 | TTS/UI 操作可能在非主线程 | 使用 `PostMessage` 切换到主线程执行 TTS 播放 |
| AI API 调用失败 | 后台线程无法获得 AI 回复 | 使用固定模板生成回复，不阻塞流程 |
| 数据库并发写入 | SQLite 写锁竞争 | 使用 WAL 模式，写操作仍需串行 |
| 内存泄漏 | std::thread detach 后生命周期管理 | 使用智能指针或确保线程短生命周期 |

## Modified Files

| 文件 | 改动内容 |
|------|---------|
| `CaptainCheckInModule.cpp/h` | 新增 `GenerateCheckinAnswerAsync` 方法；重构 `PushDanmuEvent` 锁粒度；新增数据库异步辅助方法 |
| `MiniMaxAIChatProvider.cpp/h` | 新增 `CallAPIAsync` 异步方法；`CallAPI` 保持同步调用 |
| `DeepSeekAIChatProvider.cpp/h` | 新增 `CallAPIAsync` 异步方法；`CallAPI` 保持同步调用 |
| `TextToSpeech.cpp/h` | `asyncPendingQueue_` 改为多并发支持（`activeRequestCount_` 计数器，上限3）；修改 `Tick()` 检查逻辑 |

## Open Questions

1. **回调线程问题**: `callback` 在后台线程执行，如果 callback 内有 UI 操作，需要切换到主线程。如何统一处理？
2. **错误恢复**: AI API 失败时，回调返回错误结果，调用方如何处理？是否需要重试机制？
3. **TTS 队列上限**: 最大并发请求数设为多少合适？目前建议 3-5 个。
