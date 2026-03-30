#pragma once
#include "framework.h"
#include "WriteLog.h"
#include <string>
#include <functional>
#include <stdexcept>

// 统一错误级别
enum class ErrorLevel
{
    Info,
    Warning,
    Error,
    Critical
};

// 统一错误信息结构
struct ErrorInfo
{
    ErrorLevel level;
    std::string moduleName;
    std::string message;
    int errorCode;
};

// 统一错误处理器
// 提供集中式的错误记录、通知和恢复机制
class ErrorHandler
{
public:
    // 错误回调类型
    using ErrorCallback = std::function<void(const ErrorInfo&)>;

    // 获取单例
    static ErrorHandler& Instance()
    {
        static ErrorHandler instance;
        return instance;
    }

    // 报告错误
    void Report(ErrorLevel level, const std::string& module, const std::string& message, int code = 0)
    {
        ErrorInfo info;
        info.level = level;
        info.moduleName = module;
        info.message = message;
        info.errorCode = code;

        // 记录日志
        LogError(info);

        // 通知回调
        for (const auto& cb : callbacks_)
        {
            cb(info);
        }

        // 统计
        errorCount_++;
        if (level == ErrorLevel::Critical)
            criticalCount_++;
    }

    // 注册错误回调
    void AddListener(const ErrorCallback& callback)
    {
        callbacks_.push_back(callback);
    }

    // 获取错误统计
    int GetErrorCount() const { return errorCount_; }
    int GetCriticalCount() const { return criticalCount_; }

    // 重置统计
    void ResetStats()
    {
        errorCount_ = 0;
        criticalCount_ = 0;
    }

private:
    ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    void LogError(const ErrorInfo& info)
    {
        // 使用现有日志系统
        std::wstring prefix;
        switch (info.level)
        {
        case ErrorLevel::Info: prefix = L"[INFO] "; break;
        case ErrorLevel::Warning: prefix = L"[WARN] "; break;
        case ErrorLevel::Error: prefix = L"[ERROR] "; break;
        case ErrorLevel::Critical: prefix = L"[CRITICAL] "; break;
        }

        std::wstring wmodule(info.moduleName.begin(), info.moduleName.end());
        std::wstring wmsg(info.message.begin(), info.message.end());
        TString fullMsg = prefix + wmodule + L": " + wmsg;

        switch (info.level)
        {
        case ErrorLevel::Info:
            LOG_INFO(fullMsg.c_str());
            break;
        case ErrorLevel::Warning:
            LOG_WARNING(fullMsg.c_str());
            break;
        case ErrorLevel::Error:
        case ErrorLevel::Critical:
            LOG_ERROR(fullMsg.c_str());
            break;
        }
    }

    std::vector<ErrorCallback> callbacks_;
    int errorCount_ = 0;
    int criticalCount_ = 0;
};

// 便捷宏
#define REPORT_INFO(module, msg) ErrorHandler::Instance().Report(ErrorLevel::Info, module, msg)
#define REPORT_WARNING(module, msg) ErrorHandler::Instance().Report(ErrorLevel::Warning, module, msg)
#define REPORT_ERROR(module, msg, code) ErrorHandler::Instance().Report(ErrorLevel::Error, module, msg, code)
#define REPORT_CRITICAL(module, msg, code) ErrorHandler::Instance().Report(ErrorLevel::Critical, module, msg, code)
