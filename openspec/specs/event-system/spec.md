# EventSystem - 事件系统

## 概述

C++ 模板事件系统，支持单播和多播委托模式。

## 核心 API

```cpp
template<typename... Args>
class Event {
public:
    using Handler = std::function<void(Args...)>;
    using HandlerId = size_t;

    HandlerId AddListener(const Handler& handler);
    void RemoveListener(HandlerId id);
    void Invoke(Args... args);
};
```

## 用法示例

```cpp
Event<int, std::string> onUpdate;

auto id = onUpdate.AddListener([](int value, const std::string& msg) {
    printf("Update: %d, %s\n", value, msg.c_str());
});

onUpdate.Invoke(42, "hello");

onUpdate.RemoveListener(id);
```

## 设计特点

1. **线程安全**: 公共方法未加锁，调用者需自行保证线程安全
2. **自动 ID**: 监听器 ID 自增分配
3. **轻量级**: 仅使用 `std::vector` 存储句柄对

## 文件结构

- `EventSystem.h`: 模板类定义（header-only）

## 依赖

- `<functional>`: `std::function`
- `<vector>`: 句柄存储
- `<algorithm>`: `std::remove_if`
