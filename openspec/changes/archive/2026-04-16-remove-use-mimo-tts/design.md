# 移除 USE_MIMO_TTS 宏设计

## Context

`USE_MIMO_TTS` 是编译时宏，用于可选地排除 MiMo TTS 代码。但 MiMo TTS 已经稳定运行，不需要再作为可选项。

## Decisions

### 1. 移除 framework.h 中的宏定义

删除 `framework.h:45-49`:
```cpp
// 小米MiMo TTS 编译时开关
// 设为 0 可完全排除小米MiMo相关代码（适用于无网络环境或不需要小米MiMo的场景）
#ifndef USE_MIMO_TTS
#define USE_MIMO_TTS 1
#endif
```

### 2. 移除 TextToSpeech.h 中的条件编译

删除所有：
- `#if USE_MIMO_TTS` / `#endif` 块
- 保留块内的内容，删除条件编译指令

涉及位置：
- `TextToSpeech.h:6-10` - includes
- `TextToSpeech.h:92-137` - 私有成员和方法
- `TextToSpeech.h:172-182` - 引擎降级相关

### 3. 移除 TextToSpeech.cpp 中的条件编译

删除所有 `#if USE_MIMO_TTS` / `#endif` 块，保留块内代码。

### 4. 修改 BliveManagerTests.vcxproj

移除两处 `USE_MIMO_TTS=0`:
- `BliveManagerTests.vcxproj:55`
- `BliveManagerTests.vcxproj:72`

### 5. 更新 AGENTS.md

移除 `AGENTS.md` 中关于 `USE_MIMO_TTS` 的说明。

## Files to Modify

| File | Changes |
|------|---------|
| `MonsterOrderWilds/framework.h` | 删除 USE_MIMO_TTS 宏定义 |
| `MonsterOrderWilds/TextToSpeech.h` | 移除条件编译，保留 MiMo TTS 代码 |
| `MonsterOrderWilds/TextToSpeech.cpp` | 移除条件编译，保留 MiMo TTS 代码 |
| `MonsterOrderWilds/BliveManagerTests.vcxproj` | 移除 USE_MIMO_TTS=0 |
| `AGENTS.md` | 移除相关文档 |