# TTS缓存与礼物连击优化

## Summary

本次变更加强TTS音频缓存管理和礼物连击防刷机制：
1. **TTS缓存优化**：按日期分目录存储，同一内容当天不重复保存，7天后自动清理
2. **礼物连击防刷**：全局冷却时间 + 动态连击检测，防止用户通过频繁小礼物刷屏

## Background

### 问题1：TTS缓存无内容区分
- 当前 `AudioPlayer::WriteToTempFile()` 每次创建 `mimo_tts_{timestamp}.mp3`
- 只有时间戳，无法按内容查找缓存
- 文件不断累积，关闭后无法区分哪天创建的

### 问题2：礼物连击刷屏
- 当前 `HandleSpeekSendGift()` 依赖官方 `combo_info` 超时机制
- 用户故意1个1个送礼物，每次都触发播报
- 需要冷却时间和动态连击合并机制

## Design

### Feature 1: TTS缓存管理器

#### 目录结构
```
TempAudio/
└── {YYYYMMDD}/
    └── {内容前缀}_{时间戳}.mp3
```

例：
```
TempAudio/
└── 20260404/
    └── 小明：你好啊朋_1743849600000.mp3
```

#### 缓存文件名格式
```
{内容前缀}_{时间戳}.mp3
```

内容前缀规则：
- 如果文本包含" 说："，取"说："前面的用户名 + ":" + "说："后面的前5个字
- 否则取整个文本的前5个字

例：
- "小明 说：你好啊朋友" → "小明：你好啊朋_1743849600000.mp3"
- "感谢用户赠送的礼物" → "感谢用户_1743849600000.mp3"

#### 缓存策略
1. **保存**：每次TTS请求成功后保存到当天目录，同一内容前缀当天可保存多次（时间戳区分）
2. **清理**：程序启动时删除7天前的缓存目录

#### 接口设计
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

#### 配置字段
| JSON字段 | C++字段 | 类型 | 默认值 |
|----------|---------|------|--------|
| TTS_CACHE_DAYS_TO_KEEP | ttsCacheDaysToKeep | int | 7 |

---

### Feature 2: 礼物连击优化

#### 播报规则
| 场景 | 播报时机 | 示例 |
|------|----------|------|
| 小额单次（<3个） | 立即播报 | "感谢A赠送的1个鲜花" |
| 大额单次（≥3个） | 立即播报 | "感谢A赠送的5个鲜花" |
| 官方连击 | 首报+超时尾报 | 首报："感谢A开始赠送鲜花" → 尾报："感谢A赠送的10个鲜花" |
| 动态连击 | 首报+超时尾报 | 同官方连击 |

#### 常量（写死）
| 名称 | 值 | 说明 |
|------|-----|------|
| GIFT_COOLDOWN_SECONDS | 5 | 礼物冷却时间（秒） |
| DYNAMIC_COMBO_WINDOW_SECONDS | 10 | 动态连击窗口（秒） |

#### 数据结构
```cpp
struct DynamicComboEntry {
    std::string combo_id;
    std::string uname;
    std::string gift_name;
    int gift_num;
    float combo_timeout;
    bool paid;
    bool firstReported;
    int64_t lastUpdateTime;
};
```

#### 冷却时间管理（写死常量）
```cpp
constexpr int GIFT_COOLDOWN_SECONDS = 5;
constexpr int DYNAMIC_COMBO_WINDOW_SECONDS = 10;
std::unordered_map<std::string, int64_t> giftCooldownMap_;
```

## Implementation Plan

### Phase 1: TTS缓存管理器
1. 创建 `TTSCacheManager.cpp/h`
2. 添加配置字段（ConfigManager, ConfigFieldRegistry, DataBridgeWrapper）
3. 集成到 TextToSpeech

### Phase 2: 礼物连击优化
1. 添加配置字段
2. 添加 DynamicComboEntry 结构
3. 实现冷却时间管理
4. 修改 HandleSpeekSendGift
5. 修改 Tick 处理动态连击

## Test Plan

### 单元测试
1. **TTSCacheManagerTests**
   - 测试缓存保存和查找
   - 测试按日期分目录
   - 测试7天清理逻辑

2. **TextToSpeechTests**
   - 测试冷却时间生效
   - 测试动态连击合并
   - 测试首报和尾报

## Migration

- 新增配置字段有默认值，不影响现有功能
- 缓存目录使用现有 `TempAudio` 下的日期子目录
- 礼物播报逻辑向后兼容

## Rollback

如需回滚：
1. 删除 `TTSCacheManager.cpp/h`
2. 恢复配置字段默认值
3. 回滚 TextToSpeech 中的缓存逻辑
4. 删除 DynamicComboEntry 和冷却管理代码
