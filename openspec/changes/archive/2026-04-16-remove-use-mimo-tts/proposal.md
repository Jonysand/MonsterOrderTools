# 移除 USE_MIMO_TTS 宏

## Why

`USE_MIMO_TTS` 是编译时宏，用于可选地排除 MiMo TTS 代码。但这个功能已经稳定，不需要再作为可选项。保留这个宏只会增加代码复杂度，容易引发条件编译 bug（如 bug-fix 中发现的问题）。

## What Changes

1. 移除 `framework.h` 中的 `USE_MIMO_TTS` 宏定义
2. 移除 `TextToSpeech.h/cpp` 中所有 `#if USE_MIMO_TTS` / `#endif` 条件编译块
3. 移除 `BliveManagerTests.vcxproj` 中的 `USE_MIMO_TTS=0` 定义
4. 更新 `AGENTS.md` 文档，移除相关说明

## Impact

- 简化代码，消除条件编译分支
- TTS 功能始终启用，无法在编译时禁用