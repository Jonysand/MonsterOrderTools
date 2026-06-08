# TTS缓存管理器规格

## 概述

TTS音频数据缓存管理模块，按日期分目录存储，减少重复API调用。

## 功能需求

### FR-1: 缓存存储
- 缓存文件按日期分目录存储在 `TempAudio/{YYYYMMDD}/`
- **只保存签到 AI 回复 TTS 音频**（文件名格式：`打卡_{username}_{timestamp}.mp3`）
- 一般弹幕 TTS **不缓存**，直接播放后丢弃
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
    bool SaveCheckinAudio(const std::string& username, const std::vector<uint8_t>& audioData, int64_t timestamp);
    void CleanupOldCache(int daysToKeep = 7);
};
```

## 数据流

1. 签到 TTS 请求到达 → 请求 API
2. API 返回成功 → 调用 `SaveCheckinAudio()` 保存到当天目录（`打卡_{username}_{timestamp}.mp3`）
3. 启动时 → 清理过期缓存目录
