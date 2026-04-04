# TTS缓存管理器规格

## 概述

TTS音频数据缓存管理模块，按日期分目录存储，减少重复API调用。

## 功能需求

### FR-1: 缓存存储
- 缓存文件按日期分目录存储在 `TempAudio/{YYYYMMDD}/`
- 文件名格式：`{内容前缀}_{时间戳}.mp3`
- 内容前缀规则：
  - 如果文本包含" 说："，取"说："前面的用户名 + "_" + "说："后面的前5个字
  - 否则取整个文本的前5个字
- **注意**：用户名和内容之间使用下划线 `_` 而非冒号 `:`，因为 Windows 文件系统将冒号解释为 Alternate Data Stream (ADS) 分隔符，会导致文件保存失败

### FR-2: 过期清理
- 程序启动时删除N天前的缓存目录（默认7天）

## 配置字段

| JSON字段 | C++字段 | 类型 | 默认值 |
|----------|---------|------|--------|
| TTS_CACHE_DAYS_TO_KEEP | ttsCacheDaysToKeep | int | 7 |

## 接口设计

```cpp
class TTSCacheManager {
public:
    static TTSCacheManager& Inst();
    void Initialize();
    std::wstring GetTodayCacheDir() const;
    bool SaveCachedAudio(const std::string& text, const std::vector<uint8_t>& audioData);
    void CleanupOldCache(int daysToKeep = 7);
};
```

## 数据流

1. TTS请求到达 → 请求API
2. API返回成功 → 保存到当天目录
3. 启动时 → 清理过期缓存目录
