#pragma once
#include "framework.h"
#include "MonsterDataManager.h"
#include "PriorityQueueManager.h"
#include <string>
#include <functional>
#include <vector>
#include <regex>
#include <codecvt>
#include <locale>

// 弹幕数据结构（从WebSocket接收）
struct DanmuData
{
    std::string userId;
    std::string userName;
    std::string message;
    long long timestamp = 0;
    int guardLevel = 0;      // 0-非舰长，1-总督，2-提督，3-舰长
    bool hasMedal = false;    // 是否佩戴粉丝牌
    int medalLevel = 0;      // 粉丝牌等级
    bool isPaidGift = false; // 是否付费礼物
};

// 弹幕处理结果
struct DanmuProcessResult
{
    bool matched = false;           // 是否匹配到怪物
    std::string userName;           // 用户名称
    std::string monsterName;        // 匹配的怪物名称
    int temperedLevel = 0;          // 历战等级
    bool addedToQueue = false;      // 是否加入队列
    bool shouldSpeak = false;       // 是否需要语音播报
    std::string speakText;          // 播报文本
    bool priorityUpdated = false;    // 是否更新了优先状态
};

// 弹幕处理器 - 负责弹幕解析、点怪识别、优先级处理
class DanmuProcessor
{
    DECLARE_SINGLETON(DanmuProcessor)

public:
    // 处理单条弹幕
    DanmuProcessResult ProcessDanmu(const DanmuData& danmu);

    // 批量处理弹幕
    std::vector<DanmuProcessResult> ProcessDanmuBatch(const std::vector<DanmuData>& danmus);

    // 解析JSON弹幕消息（从BliveManager接收）
    DanmuData ParseDanmuJson(const std::string& jsonStr) const;

    // 重载版本：接受已解析的json对象
    DanmuData ParseDanmuJson(const json& j) const;

    // 生成播报文本
    std::string GenerateSpeakText(const std::string& userName, const std::string& monsterName) const;

    // 设置过滤条件
    void SetOnlyMedalOrder(bool value) { onlyMedalOrder_ = value; }
    void SetOnlySpeekWearingMedal(bool value) { onlySpeekWearingMedal_ = value; }
    void SetOnlySpeekGuardLevel(int value) { onlySpeekGuardLevel_ = value; }
    void SetOnlySpeekPaidGift(bool value) { onlySpeekPaidGift_ = value; }

    // 获取过滤条件
    bool GetOnlyMedalOrder() const { return onlyMedalOrder_; }
    bool GetOnlySpeekWearingMedal() const { return onlySpeekWearingMedal_; }
    int GetOnlySpeekGuardLevel() const { return onlySpeekGuardLevel_; }
    bool GetOnlySpeekPaidGift() const { return onlySpeekPaidGift_; }

private:
    DanmuProcessor();
    ~DanmuProcessor() = default;

    void InitPatterns();

    // 检查弹幕是否符合过滤条件
    bool PassesFilter(const DanmuData& danmu) const;

    // 检查是否需要语音播报
    bool ShouldSpeak(const DanmuData& danmu) const;

    // 规范化字符串（移除空格、逗号）
    std::wstring NormalizeString(const std::wstring& input) const;

    // 尝试更新优先状态（处理"优先"、"插队"等关键字）
    bool TryUpdatePriority(const DanmuData& danmu);

    // 检查是否包含优先关键字
    bool HasPriorityKeyword(const std::wstring& text) const;

    // 过滤条件
    bool onlyMedalOrder_ = true;
    bool onlySpeekWearingMedal_ = false;
    int onlySpeekGuardLevel_ = 0;
    bool onlySpeekPaidGift_ = false;

    // 预编译的正则表达式（使用宽字符以支持UTF-8中文）
    std::vector<std::pair<std::wregex, std::wstring>> orderPatterns_;
    std::vector<std::wregex> priorityPatterns_;

    // 弹幕处理事件
public:
    using DanmuProcessedHandler = std::function<void(const DanmuProcessResult&)>;
    void AddDanmuProcessedListener(const DanmuProcessedHandler& handler);

    // 舰长弹幕事件（用于 CaptainCheckInModule 学习）
    struct CaptainDanmuEvent {
        uint64_t uid = 0;
        int32_t guardLevel = 0;
        bool hasMedal = false;
        std::string username;
        std::string content;
        int64_t serverTimestamp = 0;
        int32_t sendDate = 0;
    };
    using CaptainDanmuHandler = std::function<void(const CaptainDanmuEvent&)>;
    void AddCaptainDanmuListener(const CaptainDanmuHandler& handler);
    void Init();

private:
    std::vector<DanmuProcessedHandler> danmuProcessedListeners_;
    std::vector<CaptainDanmuHandler> captainDanmuListeners_;
    void NotifyDanmuProcessed(const DanmuProcessResult& result);
    void NotifyCaptainDanmu(const CaptainDanmuEvent& event);
};
