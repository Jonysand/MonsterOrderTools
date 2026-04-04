# ErrorHandler - 错误处理

## 概述

统一错误处理系统，提供集中式的错误记录、通知和统计机制。采用单例模式。

## 功能

### 错误级别
```cpp
enum class ErrorLevel { Info, Warning, Error, Critical };
```

### 错误信息结构
```cpp
struct ErrorInfo {
    ErrorLevel level;
    std::string moduleName;
    std::string message;
    int errorCode;
};
```

### 核心 API

```cpp
class ErrorHandler {
    static ErrorHandler& Instance();
    void Report(ErrorLevel level, const std::string& module, const std::string& message, int code = 0);
    void AddListener(const ErrorCallback& callback);
    int GetErrorCount() const;
    int GetCriticalCount() const;
    void ResetStats();
};
```

### 便捷宏
```cpp
REPORT_INFO(module, msg)
REPORT_WARNING(module, msg)
REPORT_ERROR(module, msg, code)
REPORT_CRITICAL(module, msg, code)
```

## 行为

1. 报告错误时自动记录到日志系统
2. 通知所有已注册的错误回调
3. 统计错误总数和 Critical 错误数

## 文件结构

- `ErrorHandler.h`: 接口和实现

## 依赖

- `WriteLog.h`: 日志系统
- `std::function`: 回调包装
