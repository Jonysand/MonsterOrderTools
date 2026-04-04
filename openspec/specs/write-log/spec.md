# WriteLog - 日志系统

## 概述

统一的日志记录系统，支持多级别日志输出到文件和控制台。

## 功能

### 日志级别
- `ERROR` (0): 错误日志
- `WARNING` (1): 警告日志
- `INFO` (2): 一般信息日志
- `DEBUG` (3): 调试日志（仅 Debug 配置启用）

### 核心 API

```cpp
namespace WriteLog {
    const TCHAR* GetExeDirectory();
    enum LogLevel { LogLevel_ERROR, LogLevel_WARNING, LogLevel_INFO, LogLevel_DEBUG };
    void WriteLog(LogLevel level, const TCHAR* msg, ...);
    void RecordHistory(const TCHAR* msg);
}
```

### 宏定义

```cpp
// 宽字符日志宏
LOGW_ERROR(EXPR, ...)
LOGW_WARNING(EXPR, ...)
LOGW_INFO(EXPR, ...)
LOGW_DEBUG(EXPR, ...)

// ASCII 日志宏
LOG_ERROR
LOG_WARNING
LOG_INFO
LOG_DEBUG

// 历史记录
RECORD_HISTORY(msg)
```

## 文件结构

- `WriteLog.h`: 接口定义和宏定义
- `WriteLog.cpp`: 实现

## 依赖

- Windows API: `MultiByteToWideChar`, `WideCharToMultiByte`
- 标准库: `<stdarg.h>`, `<stdio.h>`

## 配置

日志级别通过编译配置控制：
- Debug 配置: 启用 `LOG_DEBUG`
- Release 配置: `LOG_DEBUG` 为空宏
