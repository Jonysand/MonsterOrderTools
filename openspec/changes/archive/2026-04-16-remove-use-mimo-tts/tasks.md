# 移除 USE_MIMO_TTS 宏实现计划

**Goal:** 移除所有 `USE_MIMO_TTS` 条件编译，TTS 功能始终启用

**Architecture:** 直接删除条件编译宏，保留 MiMo TTS 代码

**Tech Stack:** C++ (framework.h, TextToSpeech.h, TextToSpeech.cpp), XML (vcxproj)

---

## Task 1: 移除 framework.h 中的 USE_MIMO_TTS 宏定义

**Files:**
- Modify: `MonsterOrderWilds/framework.h:45-49`

**Step 1: 删除宏定义**

删除：
```cpp
// 小米MiMo TTS 编译时开关
// 设为 0 可完全排除小米MiMo相关代码（适用于无网络环境或不需要小米MiMo的场景）
#ifndef USE_MIMO_TTS
#define USE_MIMO_TTS 1
#endif
```

**Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 2: 移除 TextToSpeech.h 中的条件编译

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.h`

**Step 1: 移除 include 块的 #if USE_MIMO_TTS**

将：
```cpp
#if USE_MIMO_TTS
#include "TTSProvider.h"
#include "AudioPlayer.h"
#include "TTSCacheManager.h"
#endif
```
改为：
```cpp
#include "TTSProvider.h"
#include "AudioPlayer.h"
#include "TTSCacheManager.h"
```

**Step 2: 移除私有成员区的 #if USE_MIMO_TTS 块**

将 `TextToSpeech.h` 中所有 `#if USE_MIMO_TTS` 到对应 `#endif` 之间的条件编译指令删除，保留块内代码。

**Step 3: 移除引擎降级相关的 #if USE_MIMO_TTS 块**

将 `TextToSpeech.h` 中 `isFallback`、`fallbackReason` 等成员的 `#if USE_MIMO_TTS` 块展开。

**Step 4: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 3: 移除 TextToSpeech.cpp 中的条件编译

**Files:**
- Modify: `MonsterOrderWilds/TextToSpeech.cpp`

**Step 1: 移除所有 #if USE_MIMO_TTS / #endif 块**

删除所有条件编译指令，保留块内代码。

**Step 2: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 4: 移除 BliveManagerTests.vcxproj 中的 USE_MIMO_TTS=0

**Files:**
- Modify: `MonsterOrderWilds/BliveManagerTests.vcxproj`

**Step 1: 移除 Debug 配置中的 USE_MIMO_TTS=0**

在 `PreprocessorDefinitions` 中删除 `USE_MIMO_TTS=0;`

**Step 2: 移除 Release 配置中的 USE_MIMO_TTS=0**

在 `PreprocessorDefinitions` 中删除 `USE_MIMO_TTS=0;`

**Step 3: 验证编译**

Run: `powershell -Command "& 'D:\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'D:\VisualStudioProjects\JonysandMHDanmuTools\JonysandMHDanmuTools.sln' -p:Configuration=Release -p:Platform=x64 -t:Build -m"`
Expected: `0 个错误`

---

## Task 5: 更新 AGENTS.md

**Files:**
- Modify: `AGENTS.md`

**Step 1: 删除编译宏说明**

删除关于 `USE_MIMO_TTS` 的说明。

---

## 文件清单

| 文件 | 修改类型 |
|------|---------|
| `MonsterOrderWilds/framework.h` | 删除 USE_MIMO_TTS 宏定义 |
| `MonsterOrderWilds/TextToSpeech.h` | 移除条件编译 |
| `MonsterOrderWilds/TextToSpeech.cpp` | 移除条件编译 |
| `MonsterOrderWilds/BliveManagerTests.vcxproj` | 移除 USE_MIMO_TTS=0 |
| `AGENTS.md` | 移除相关文档 |