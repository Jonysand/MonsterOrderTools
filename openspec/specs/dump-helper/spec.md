# DumpHelper - 崩溃转储

## 概述

崩溃转储辅助模块，支持生成 MiniDump 或 FullDump。

## 核心 API

```cpp
class DumpHelper {
public:
    enum DUMP_TYPE { FULL_DUMP, MINI_DUMP };
    
    static void Init(const char* dumpFilename, DUMP_TYPE dtype);
    static int HandleCrash(void* ExceptionInfo);
    static void SetCrashCallback(std::function<int()>&& d);
    static void AppendDumpInfo(const TString& info);
    static const TString& GetExtDumpInfo();
};
```

## 转储类型

| 类型 | 说明 |
|------|------|
| `FULL_DUMP` | 完整内存转储 |
| `MINI_DUMP` | 最小化转储（默认） |

## 功能

1. **初始化**: 设置转储文件名和类型
2. **异常处理**: `HandleCrash` 处理程序崩溃
3. **扩展信息**: 支持追加自定义诊断信息
4. **崩溃回调**: 可注册自定义崩溃处理逻辑

## 文件结构

- `DumpHelper.h`: 接口声明
- `DumpHelper.cpp`: 实现

## 依赖

- Windows API: `MiniDumpWriteDump`, `GetExceptionInformation`
