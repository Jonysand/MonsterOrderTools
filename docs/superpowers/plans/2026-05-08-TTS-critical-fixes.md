# TTS Critical Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 3 critical issues identified in code review: SAPI queue head blocking, startTime semantic overload, and activeRequestCount_ race condition.

**Architecture:** Modify `AsyncTTSRequest` struct to separate time fields, adjust `ProcessAsyncTTS` scheduling loop to only block SAPI requests (not all requests), and add state consistency checks in failure handling to prevent race conditions between timeout and callback threads.

**Tech Stack:** C++, Win32, Visual Studio 2022, MSBuild

---

## File Structure

### Files to Modify

| File | Responsibility | Changes |
|------|---------------|---------|
| `MonsterOrderWilds/TextToSpeech.h` | AsyncTTSRequest struct definition | Add `requestStartTime`, `playbackStartTime`, `retryAfterTime`; remove overloaded `startTime` usage |
| `MonsterOrderWilds/TextToSpeech.cpp` | TTS state machine implementation | Fix SAPI scheduling loop, update all `startTime` references, add race condition guards |

### No New Files

All changes are localized to the existing TTS subsystem.

---

## Task 1: Fix startTime Semantic Overload

**Problem:** `AsyncTTSRequest::startTime` carries 4 semantics: request start, playback start, retry deadline, timeout baseline. When retry sets it to `now + 500ms`, timeout calculations become negative.

**Solution:** Split into 3 dedicated fields.

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h:35-52`
- Modify: `MonsterOrderWilds/TextToSpeech.cpp` (all `startTime` references)

### Step 1: Modify AsyncTTSRequest struct in header

Open `MonsterOrderWilds/TextToSpeech.h` and replace the `startTime` field:

```cpp
// Before:
	std::chrono::steady_clock::time_point startTime;  // 状态开始时间

// After:
	std::chrono::steady_clock::time_point requestStartTime;  // 请求开始时间（用于API超时判定）
	std::chrono::steady_clock::time_point playbackStartTime; // 播放开始时间（用于播放超时判定）
	std::chrono::steady_clock::time_point retryAfterTime;    // 重试等待截止时间
```

### Step 2: Update HandleSpeekDm

In `MonsterOrderWilds/TextToSpeech.cpp:198`, change:

```cpp
// Before:
		reqPtr->startTime = std::chrono::steady_clock::now();

// After:
		reqPtr->requestStartTime = std::chrono::steady_clock::now();
		reqPtr->playbackStartTime = std::chrono::steady_clock::time_point(); // 未开始播放
		reqPtr->retryAfterTime = std::chrono::steady_clock::time_point();    // 未设置重试
```

### Step 3: Update Speak method

In `MonsterOrderWilds/TextToSpeech.cpp:380`, change:

```cpp
// Before:
	reqPtr->startTime = std::chrono::steady_clock::now();

// After:
	reqPtr->requestStartTime = std::chrono::steady_clock::now();
	reqPtr->playbackStartTime = std::chrono::steady_clock::time_point();
	reqPtr->retryAfterTime = std::chrono::steady_clock::time_point();
```

### Step 4: Update SpeakCheckinTTS method

In `MonsterOrderWilds/TextToSpeech.cpp:410`, change:

```cpp
// Before:
	reqPtr->startTime = std::chrono::steady_clock::now();

// After:
	reqPtr->requestStartTime = std::chrono::steady_clock::now();
	reqPtr->playbackStartTime = std::chrono::steady_clock::time_point();
	reqPtr->retryAfterTime = std::chrono::steady_clock::time_point();
```

### Step 5: Update SpeakWithMimoAsync method

In `MonsterOrderWilds/TextToSpeech.cpp:626`, change:

```cpp
// Before:
	reqPtr->startTime = std::chrono::steady_clock::now();

// After:
	reqPtr->requestStartTime = std::chrono::steady_clock::now();
	reqPtr->playbackStartTime = std::chrono::steady_clock::time_point();
	reqPtr->retryAfterTime = std::chrono::steady_clock::time_point();
```

### Step 6: Update ProcessAsyncTTS retry check

In `MonsterOrderWilds/TextToSpeech.cpp:687-693`, change:

```cpp
// Before:
				if ((*it)->retryCount > 0) {
					auto now = std::chrono::steady_clock::now();
					if (now < (*it)->startTime) {
						auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>((*it)->startTime - now).count();
						LOG_DEBUG(TEXT("TTS Async: Request waiting retry interval (%lld ms), skipping: %s"), waitMs, (*it)->text.c_str());
						continue;
					}
				}

// After:
				if ((*it)->retryCount > 0 && (*it)->retryAfterTime != std::chrono::steady_clock::time_point()) {
					auto now = std::chrono::steady_clock::now();
					if (now < (*it)->retryAfterTime) {
						auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>((*it)->retryAfterTime - now).count();
						LOG_DEBUG(TEXT("TTS Async: Request waiting retry interval (%lld ms), skipping: %s"), waitMs, (*it)->text.c_str());
						continue;
					}
				}
```

### Step 7: Update ProcessAsyncTTS request start

In `MonsterOrderWilds/TextToSpeech.cpp:714`, change:

```cpp
// Before:
				(*it)->startTime = std::chrono::steady_clock::now();

// After:
				(*it)->requestStartTime = std::chrono::steady_clock::now();
```

### Step 8: Update ProcessPendingRequestInternal for MiMo

In `MonsterOrderWilds/TextToSpeech.cpp:905`, change:

```cpp
// Before:
	req.startTime = std::chrono::steady_clock::now();

// After:
	req.requestStartTime = std::chrono::steady_clock::now();
```

### Step 9: Update ProcessRequestingStateInternal timeout check

In `MonsterOrderWilds/TextToSpeech.cpp:923-934`, change:

```cpp
// Before:
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.startTime).count();

	if (elapsed >= API_TIMEOUT_SECONDS) {
		LOG_WARNING(TEXT("TTS Async: API request timeout (%lld seconds)"), (long long)elapsed);
		if (elapsed >= API_TIMEOUT_SECONDS * 3) {
			HandleRequestFailureInternal(req);
		}
	}

// After:
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.requestStartTime).count();

	if (elapsed >= API_TIMEOUT_SECONDS) {
		LOG_WARNING(TEXT("TTS Async: API request timeout (%lld seconds)"), (long long)elapsed);
		if (elapsed >= API_TIMEOUT_SECONDS * 3) {
			HandleRequestFailureInternal(req);
		}
	}
```

### Step 10: Update ProcessPlayingStateInternal playback timeout

In `MonsterOrderWilds/TextToSpeech.cpp:970` and `993`, change:

```cpp
// Line 970 - Before:
		req.startTime = std::chrono::steady_clock::now();  // 重置超时计时

// Line 970 - After:
		req.playbackStartTime = std::chrono::steady_clock::now();  // 重置播放超时计时

// Line 993 - Before:
			auto playbackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.startTime).count();

// Line 993 - After:
			auto playbackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.playbackStartTime).count();
```

### Step 11: Update HandleRequestFailureInternal retry logic

In `MonsterOrderWilds/TextToSpeech.cpp:1061`, change:

```cpp
// Before:
		req.startTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(RETRY_INTERVAL_MS);

// After:
		req.retryAfterTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(RETRY_INTERVAL_MS);
		req.requestStartTime = std::chrono::steady_clock::now(); // 重置请求开始时间，避免重试后超时计算异常
```

### Step 12: Verify compilation

Run:
```bash
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: Build succeeds with 0 errors.

### Step 13: Run git diff to verify changes

```bash
git diff MonsterOrderWilds/TextToSpeech.cpp MonsterOrderWilds/TextToSpeech.h
```

Verify: Only `startTime` references changed to dedicated fields, no other modifications.

---

## Task 2: Fix SAPI Serialization Queue Head Blocking

**Problem:** When a Pending SAPI request is blocked by an ongoing Playing SAPI request, the `foundPending = true; break;` causes the outer `while` loop to exit, blocking all subsequent non-SAPI requests.

**Solution:** Only set `foundPending = true` when a request is actually started. If a request is skipped due to SAPI serialization, continue searching for the next eligible request.

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp:681-726`

### Step 1: Modify the Pending request scheduling loop

In `MonsterOrderWilds/TextToSpeech.cpp`, replace the second loop (lines 681-726):

```cpp
// Before:
	while (activeRequestCount_ < MAX_CONCURRENT_TTS && !asyncPendingQueue_.empty()) {
		bool foundPending = false;
		for (auto it = asyncPendingQueue_.begin(); it != asyncPendingQueue_.end(); ++it) {
			if ((*it)->state == AsyncTTSState::Pending) {
				// ... retry check ...
				// ... SAPI check ...
				// ... if blocked, continue ...
				
				(*it)->startTime = std::chrono::steady_clock::now();
				activeRequestCount_++;
				LOG_INFO(TEXT("TTS Async: Starting new request for: %s (active: %d)"),
					(*it)->text.c_str(), activeRequestCount_.load());
				ProcessPendingRequestInternal(it);
				foundPending = true;
				break;
			}
		}
		if (!foundPending) {
			break;
		}
	}

// After:
	while (activeRequestCount_ < MAX_CONCURRENT_TTS && !asyncPendingQueue_.empty()) {
		bool startedAny = false;
		for (auto it = asyncPendingQueue_.begin(); it != asyncPendingQueue_.end(); ++it) {
			if ((*it)->state == AsyncTTSState::Pending) {
				// 检查重试间隔：如果请求正在等待重试间隔，跳过
				if ((*it)->retryCount > 0 && (*it)->retryAfterTime != std::chrono::steady_clock::time_point()) {
					auto now = std::chrono::steady_clock::now();
					if (now < (*it)->retryAfterTime) {
						auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>((*it)->retryAfterTime - now).count();
						LOG_DEBUG(TEXT("TTS Async: Request waiting retry interval (%lld ms), skipping: %s"), waitMs, (*it)->text.c_str());
						continue; // 跳过这个请求，继续查找下一个 Pending
					}
				}
				// SAPI 请求需要串行播放：检查是否已有 SAPI 请求正在播放
				bool willUseSapi = ((*it)->engineType == TTSEngineType::SAPI) ||
					isFallback ||
					(ttsProvider && ttsProvider->GetProviderName() == "sapi");
				if (willUseSapi) {
					bool hasSapiPlaying = false;
					for (auto& req : asyncPendingQueue_) {
						if (req->engineType == TTSEngineType::SAPI && req->state == AsyncTTSState::Playing) {
							hasSapiPlaying = true;
							break;
						}
					}
					if (hasSapiPlaying) {
						LOG_DEBUG(TEXT("TTS Async: SAPI request queued, waiting for previous to complete: %s"),
							(*it)->text.c_str());
						continue; // 跳过这个SAPI请求，继续查找下一个非SAPI的Pending请求
					}
				}

				(*it)->requestStartTime = std::chrono::steady_clock::now();
				activeRequestCount_++;
				LOG_INFO(TEXT("TTS Async: Starting new request for: %s (active: %d)"),
					(*it)->text.c_str(), activeRequestCount_.load());
				ProcessPendingRequestInternal(it);
				startedAny = true;
				break; // 启动了一个请求后退出内层循环，让外层while检查并发上限
			}
		}
		if (!startedAny) {
			break; // 没有启动任何请求（所有Pending都被跳过或没有Pending），退出外层循环
		}
	}
```

### Step 2: Verify compilation

Run:
```bash
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: Build succeeds with 0 errors.

### Step 3: Run git diff to verify changes

```bash
git diff MonsterOrderWilds/TextToSpeech.cpp
```

Verify: Only the scheduling loop changed. The key difference is `continue` for SAPI-blocked requests no longer exits the entire scheduling loop.

---

## Task 3: Fix activeRequestCount_ Race Condition

**Problem:** `ProcessRequestingStateInternal` (timeout thread) and HTTP callback may race on the same request. Timeout thread calls `HandleRequestFailureInternal` while callback thread sets state to Playing.

**Solution:** Add state consistency check in `HandleRequestFailureInternal` and `ProcessRequestingStateInternal` to ensure we only act on requests in expected states.

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp:908-935` (ProcessRequestingStateInternal)
- Modify: `MonsterOrderWilds/TextToSpeech.cpp:1052-1093` (HandleRequestFailureInternal)

### Step 1: Add state guard in ProcessRequestingStateInternal

In `MonsterOrderWilds/TextToSpeech.cpp:916-934`, change:

```cpp
// Before:
	// 如果已经有音频数据，说明回调已执行，状态改为Playing让下次Tick处理播放
	if (!req.audioData.empty()) {
		req.state = AsyncTTSState::Playing;
		LOG_INFO(TEXT("TTS Async: State changed to Playing"));
		return;
	}

	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.requestStartTime).count();

	if (elapsed >= API_TIMEOUT_SECONDS) {
		LOG_WARNING(TEXT("TTS Async: API request timeout (%lld seconds)"), (long long)elapsed);
		if (elapsed >= API_TIMEOUT_SECONDS * 3) {
			HandleRequestFailureInternal(req);
		}
	}

// After:
	// 如果已经有音频数据，说明回调已执行，状态改为Playing让下次Tick处理播放
	if (!req.audioData.empty()) {
		req.state = AsyncTTSState::Playing;
		LOG_INFO(TEXT("TTS Async: State changed to Playing"));
		return;
	}

	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.requestStartTime).count();

	if (elapsed >= API_TIMEOUT_SECONDS) {
		LOG_WARNING(TEXT("TTS Async: API request timeout (%lld seconds)"), (long long)elapsed);
		if (elapsed >= API_TIMEOUT_SECONDS * 3) {
			// Race condition guard: double-check state and audioData before handling failure
			// HTTP callback might have just completed between the audioData check above and now
			if (req.state == AsyncTTSState::Requesting && req.audioData.empty()) {
				HandleRequestFailureInternal(req);
			} else {
				LOG_INFO(TEXT("TTS Async: Request state changed or audioData arrived during timeout check, skipping failure handling"));
			}
		}
	}
```

### Step 2: Add state guard in HandleRequestFailureInternal

In `MonsterOrderWilds/TextToSpeech.cpp:1052-1093`, change:

```cpp
// Before:
bool TTSManager::HandleRequestFailureInternal(AsyncTTSRequest& req)
{
	// 注意：此函数在ProcessAsyncTTS持有asyncMutex_时被调用，不要再获取锁
	// 返回值：true = 请求真正失败（不再重试），false = 正在重试

	// 重试逻辑
	if (req.retryCount < MAX_RETRY_COUNT) {
		req.retryCount++;
		req.state = AsyncTTSState::Pending;  // 重置为Pending，重新请求
		req.retryAfterTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(RETRY_INTERVAL_MS);
		req.requestStartTime = std::chrono::steady_clock::now(); // 重置请求开始时间
		activeRequestCount_--;  // Bug #3 fix: decrement to avoid double-counting when second loop picks it up
		LOG_WARNING(TEXT("TTS Async: Retrying request (%d/%d) after %dms"), req.retryCount, MAX_RETRY_COUNT, RETRY_INTERVAL_MS);
		return false;
	}

// After:
bool TTSManager::HandleRequestFailureInternal(AsyncTTSRequest& req)
{
	// 注意：此函数在ProcessAsyncTTS持有asyncMutex_时被调用，不要再获取锁
	// 返回值：true = 请求真正失败（不再重试），false = 正在重试

	// Race condition guard: only handle failure if request is still in an expected state
	// If state has already changed to Playing/Completed/Failed (by callback or other logic), skip
	if (req.state != AsyncTTSState::Requesting && req.state != AsyncTTSState::Playing) {
		LOG_DEBUG(TEXT("TTS Async: HandleRequestFailureInternal called but request state is %d, not Requesting/Playing. Skipping."), (int)req.state);
		return true; // Treat as already handled (failed or completed)
	}

	// If audioData has arrived (callback completed just before we got the lock), don't fail
	if (!req.audioData.empty()) {
		LOG_INFO(TEXT("TTS Async: audioData arrived before failure handling, converting to Playing state"));
		req.state = AsyncTTSState::Playing;
		return false; // Not a failure, will continue as Playing
	}

	// 重试逻辑
	if (req.retryCount < MAX_RETRY_COUNT) {
		req.retryCount++;
		req.state = AsyncTTSState::Pending;  // 重置为Pending，重新请求
		req.retryAfterTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(RETRY_INTERVAL_MS);
		req.requestStartTime = std::chrono::steady_clock::now(); // 重置请求开始时间
		activeRequestCount_--;  // Bug #3 fix: decrement to avoid double-counting when second loop picks it up
		LOG_WARNING(TEXT("TTS Async: Retrying request (%d/%d) after %dms"), req.retryCount, MAX_RETRY_COUNT, RETRY_INTERVAL_MS);
		return false;
	}
```

### Step 3: Verify compilation

Run:
```bash
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: Build succeeds with 0 errors.

### Step 4: Run git diff to verify changes

```bash
git diff MonsterOrderWilds/TextToSpeech.cpp
```

Verify: State guards added in both functions. No other changes.

---

## Final Verification

### Step 1: Full compilation check

Run:
```bash
powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"
```

Expected: Build succeeds with 0 errors.

### Step 2: Complete git diff review

Run:
```bash
git diff MonsterOrderWilds/TextToSpeech.cpp MonsterOrderWilds/TextToSpeech.h
```

Verify checklist:
- [ ] `startTime` replaced with `requestStartTime`, `playbackStartTime`, `retryAfterTime` in all locations
- [ ] No remaining references to `startTime` in TextToSpeech.cpp or TextToSpeech.h
- [ ] SAPI scheduling loop uses `continue` for blocked requests instead of exiting outer loop
- [ ] `foundPending` renamed to `startedAny` with correct semantics
- [ ] State consistency guards added in `ProcessRequestingStateInternal` and `HandleRequestFailureInternal`
- [ ] `activeRequestCount_` decrements only happen after state checks
- [ ] No unrelated changes

### Step 3: User review

Show the complete diff to the user for final approval before any commit.

---

## Self-Review Checklist

### 1. Spec Coverage

| Review Finding | Task | Status |
|---------------|------|--------|
| SAPI串行化导致队列头部阻塞 | Task 2 | ✅ Covered |
| startTime语义重载导致超时判断失效 | Task 1 | ✅ Covered |
| activeRequestCount_竞态条件 | Task 3 | ✅ Covered |

### 2. Placeholder Scan

- [ ] No "TBD", "TODO", "implement later"
- [ ] No "Add appropriate error handling" without code
- [ ] No "Similar to Task N" references
- [ ] All code blocks contain actual code

### 3. Type Consistency

- [ ] `requestStartTime` used consistently for API timeout
- [ ] `playbackStartTime` used consistently for playback timeout
- [ ] `retryAfterTime` used consistently for retry interval
- [ ] `startedAny` variable name consistent in Task 2
- [ ] State check conditions consistent (`req.state == AsyncTTSState::Requesting`)

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-05-08-TTS-critical-fixes.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
