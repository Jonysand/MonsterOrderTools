#include "framework.h"
#include "DanmuProcessor.h"
#include "WriteLog.h"

namespace {
    uint64_t HashStringToUint64(const std::string& str) {
        if (str.empty()) return 0;
        // FNV-1a hash
        uint64_t hash = 14695981039346656037ull;
        for (char c : str) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ull;
        }
        return hash;
    }
}

DEFINE_SINGLETON(DanmuProcessor)

DanmuProcessor::DanmuProcessor()
{
    InitPatterns();
}

void DanmuProcessor::Init()
{
}

void DanmuProcessor::InitPatterns()
{
    orderPatterns_.push_back({ std::wregex(L"^点怪"), L"点怪" });
    orderPatterns_.push_back({ std::wregex(L"^点个"), L"点个" });
    orderPatterns_.push_back({ std::wregex(L"^点只"), L"点只" });
    orderPatterns_.push_back({ std::wregex(L"^點怪"), L"點怪" });
    orderPatterns_.push_back({ std::wregex(L"^點個"), L"點個" });
    orderPatterns_.push_back({ std::wregex(L"^點隻"), L"點隻" });

    priorityPatterns_.push_back(std::wregex(L"优先"));
    priorityPatterns_.push_back(std::wregex(L"插队"));
    priorityPatterns_.push_back(std::wregex(L"優先"));
    priorityPatterns_.push_back(std::wregex(L"插隊"));
}

DanmuProcessResult DanmuProcessor::ProcessDanmu(const DanmuData& danmu)
{
    DanmuProcessResult result;

    std::wstring wuserName = Utf8ToWstring(danmu.userName);
    std::wstring wmessage = Utf8ToWstring(danmu.message);
    LOGW_DEBUG(L"[DanmuProcessor] ProcessDanmu: user=%s, msg=%s, hasMedal=%d, guardLevel=%d",
        wuserName.c_str(), wmessage.c_str(), danmu.hasMedal, danmu.guardLevel);

    // 先通知打卡模块（独立于过滤逻辑，打卡功能不受播报过滤配置影响）
#if TEST_CAPTAIN_REPLY_LOCAL
    {
        CaptainDanmuEvent capEvent;
        capEvent.uid = HashStringToUint64(danmu.userId);
        capEvent.guardLevel = 3;  // 模拟舰长
        capEvent.hasMedal = danmu.hasMedal;
        capEvent.username = danmu.userName;
        capEvent.content = danmu.message;
        capEvent.serverTimestamp = danmu.timestamp;

        std::time_t timeSec = danmu.timestamp;
        std::tm tmResult = {};
        if (localtime_s(&tmResult, &timeSec) == 0) {
            capEvent.sendDate = (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
        } else {
            capEvent.sendDate = 0;
        }

        NotifyCaptainDanmu(capEvent);
    }
#else
    if (danmu.guardLevel != 0 || danmu.hasMedal)
    {
        CaptainDanmuEvent capEvent;
        capEvent.uid = HashStringToUint64(danmu.userId);
        capEvent.guardLevel = danmu.guardLevel;
        capEvent.hasMedal = danmu.hasMedal;
        capEvent.username = danmu.userName;
        capEvent.content = danmu.message;
        capEvent.serverTimestamp = danmu.timestamp;

        std::time_t timeSec = danmu.timestamp;
        std::tm tmResult = {};
        if (localtime_s(&tmResult, &timeSec) == 0) {
            capEvent.sendDate = (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
        } else {
            capEvent.sendDate = 0;
        }

        LOG_DEBUG(TEXT("[DanmuProcessor] Notifying CaptainDanmu: username=%s, guardLevel=%d, hasMedal=%d, content=%s"),
            capEvent.username.c_str(), capEvent.guardLevel, capEvent.hasMedal, capEvent.content.c_str());

        NotifyCaptainDanmu(capEvent);
    }
#endif

    if (!PassesFilter(danmu))
    {
        LOGW_DEBUG(L"[DanmuProcessor] PassesFilter failed");
        return result;
    }

    std::wstring wmsg = Utf8ToWstring(danmu.message);
    std::wstring wnormalizedMsg = NormalizeString(wmsg);
    
    LOGW_DEBUG(L"[DanmuProcessor] normalizedMsg length=%d", (int)wnormalizedMsg.length());

    if (TryUpdatePriority(danmu))
    {
        result.priorityUpdated = true;
        NotifyDanmuProcessed(result);
        return result;
    }

    if (danmu.hasMedal && !danmu.userId.empty())
    {
        PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
        if (queueMgr->Contains(danmu.userId))
        {
            LOGW_DEBUG(L"[DanmuProcessor] user already in queue");
            NotifyDanmuProcessed(result);
            return result;
        }
    }

    std::string monsterName;
    int temperedLevel = 0;
    bool matched = false;

    LOGW_DEBUG(L"[DanmuProcessor] Starting pattern loop, orderPatterns_.size()=%d", (int)orderPatterns_.size());
    for (const auto& [pattern, keyword] : orderPatterns_)
    {
        std::wsmatch match;
        bool searchResult = std::regex_search(wnormalizedMsg, match, pattern);
        LOGW_DEBUG(L"[DanmuProcessor] pattern search result: %d", searchResult);
        if (searchResult)
        {
            std::wstring subString = wnormalizedMsg.substr(match.position() + keyword.length());
            LOGW_DEBUG(L"[DanmuProcessor] pattern matched, subString length=%d", (int)subString.length());

            for (const auto& priorityRegex : priorityPatterns_)
            {
                std::wsmatch priorityMatch;
                if (std::regex_search(subString, priorityMatch, priorityRegex) && danmu.guardLevel > 0)
                {
                    subString = std::regex_replace(subString, priorityRegex, L"");
                }
            }

            std::string subStringUtf8 = WstringToUtf8(subString);
            MonsterDataManager* monsterMgr = MonsterDataManager::Inst();
            MonsterMatchResult matchResult = monsterMgr->GetMatchedMonsterName(subStringUtf8);

            if (matchResult.HasMatch())
            {
                monsterName = matchResult.monsterName;
                temperedLevel = matchResult.temperedLevel;
                matched = true;
                std::wstring wmonsterName = Utf8ToWstring(monsterName);
                LOGW_DEBUG(L"[DanmuProcessor] Monster matched: name=%s, temperedLevel=%d", wmonsterName.c_str(), temperedLevel);
                break;
            }
            else
            {
                std::wstring wsubStringUtf8 = Utf8ToWstring(subStringUtf8);
                LOGW_DEBUG(L"[DanmuProcessor] No monster match for subString=%s", wsubStringUtf8.c_str());
                continue;
            }
        }
    }
    LOGW_DEBUG(L"[DanmuProcessor] Pattern loop finished, matched=%d", matched);

    if (matched && !monsterName.empty())
    {
        std::wstring wmonsterName = Utf8ToWstring(monsterName);
        LOGW_DEBUG(L"[DanmuProcessor] Enqueueing: user=%s, monster=%s", wuserName.c_str(), wmonsterName.c_str());
        result.matched = true;
        result.userName = danmu.userName;
        result.monsterName = monsterName;
        result.temperedLevel = temperedLevel;

        QueueNodeData node;
        node.userId = danmu.userId;
        node.userName = danmu.userName;
        node.monsterName = monsterName;
        node.timeStamp = danmu.timestamp;
        node.priority = danmu.guardLevel > 0;
        node.guardLevel = danmu.guardLevel;
        node.temperedLevel = temperedLevel;

        PriorityQueueManager::Inst()->Enqueue(node);
        result.addedToQueue = true;
        LOGW_DEBUG(L"[DanmuProcessor] Enqueued successfully, addedToQueue=%d", result.addedToQueue);

        if (ShouldSpeak(danmu))
        {
            result.shouldSpeak = true;
            result.speakText = GenerateSpeakText(danmu.userName, monsterName);
        }
    }
    else
    {
        std::wstring wmonsterName = Utf8ToWstring(monsterName);
        LOGW_DEBUG(L"[DanmuProcessor] No match found, matched=%d, monsterName=%s", matched, wmonsterName.c_str());
    }

    NotifyDanmuProcessed(result);

    return result;
}

std::vector<DanmuProcessResult> DanmuProcessor::ProcessDanmuBatch(const std::vector<DanmuData>& danmus)
{
    std::vector<DanmuProcessResult> results;
    results.reserve(danmus.size());

    for (const auto& danmu : danmus)
    {
        results.push_back(ProcessDanmu(danmu));
    }

    return results;
}

DanmuData DanmuProcessor::ParseDanmuJson(const std::string& jsonStr) const
{
    DanmuData data;
    try
    {
        json j = json::parse(jsonStr);
        return ParseDanmuJson(j);
    }
    catch (const json::parse_error& e)
    {
        LOG_ERROR(TEXT("[ParseDanmuJson] JSON parse error at byte %d: %s"), (int)e.byte, e.what());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("[ParseDanmuJson] ParseDanmuJson exception: %s"), e.what());
    }
    return data;
}

DanmuData DanmuProcessor::ParseDanmuJson(const json& j) const
{
    DanmuData data;
    try
    {
        json dataObj = j;
        if (j.contains("cmd") && j.contains("data"))
        {
            std::string cmd = j["cmd"].get<std::string>();
            if (cmd != "LIVE_OPEN_PLATFORM_DM")
                return data;
            dataObj = j["data"];
        }

        if (dataObj.contains("open_id"))
            data.userId = dataObj["open_id"].get<std::string>();
        else if (dataObj.contains("uid"))
            data.userId = std::to_string(dataObj["uid"].get<int64_t>());
        if (dataObj.contains("uname"))
            data.userName = dataObj["uname"].get<std::string>();
        if (dataObj.contains("msg")) {
            data.message = dataObj["msg"].get<std::string>();
            LOG_DEBUG(TEXT("[ParseDanmuJson] msg extracted: length=%d"), (int)data.message.length());
        }
        if (dataObj.contains("timestamp"))
            data.timestamp = dataObj["timestamp"].get<long long>();
        else if (dataObj.contains("send_time"))
            data.timestamp = dataObj["send_time"].get<long long>();

        if (dataObj.contains("fans_medal_wearing_status") && dataObj["fans_medal_wearing_status"].get<bool>())
        {
            data.hasMedal = true;
            if (dataObj.contains("fans_medal_level"))
                data.medalLevel = dataObj["fans_medal_level"].get<int>();
        }

        if (dataObj.contains("guard_level"))
            data.guardLevel = dataObj["guard_level"].get<int>();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("[ParseDanmuJson] ParseDanmuJson (json overload) exception: %s"), e.what());
    }
    return data;
}

std::string DanmuProcessor::GenerateSpeakText(const std::string& userName, const std::string& monsterName) const
{
    return userName + "点怪" + monsterName;
}

bool DanmuProcessor::PassesFilter(const DanmuData& danmu) const
{
    if (onlyMedalOrder_ && !danmu.hasMedal)
        return false;

    return true;
}

bool DanmuProcessor::ShouldSpeak(const DanmuData& danmu) const
{
    if (onlySpeekWearingMedal_ && !danmu.hasMedal)
        return false;

    if (onlySpeekGuardLevel_ > 0 && danmu.guardLevel != onlySpeekGuardLevel_)
        return false;

    if (onlySpeekPaidGift_ && !danmu.isPaidGift)
        return false;

    return true;
}

std::wstring DanmuProcessor::NormalizeString(const std::wstring& input) const
{
    std::wstring result = input;
    result = std::regex_replace(result, std::wregex(L"[ ,，]"), L"");
    return result;
}

bool DanmuProcessor::HasPriorityKeyword(const std::wstring& text) const
{
    for (const auto& pattern : priorityPatterns_)
    {
        if (std::regex_search(text, pattern))
            return true;
    }
    return false;
}

bool DanmuProcessor::TryUpdatePriority(const DanmuData& danmu)
{
    std::wstring wmsg = Utf8ToWstring(danmu.message);
    std::wstring wnormalizedMsg = NormalizeString(wmsg);

    if (!HasPriorityKeyword(wnormalizedMsg))
        return false;

    if (danmu.guardLevel <= 0)
        return false;

    PriorityQueueManager* queueMgr = PriorityQueueManager::Inst();
    if (!queueMgr->Contains(danmu.userId))
        return false;

    return true;
}

void DanmuProcessor::AddDanmuProcessedListener(const DanmuProcessedHandler& handler)
{
    danmuProcessedListeners_.push_back(handler);
}

void DanmuProcessor::NotifyDanmuProcessed(const DanmuProcessResult& result)
{
    for (const auto& handler : danmuProcessedListeners_)
    {
        handler(result);
    }
}

void DanmuProcessor::AddCaptainDanmuListener(const CaptainDanmuHandler& handler)
{
    captainDanmuListeners_.push_back(handler);
}

void DanmuProcessor::NotifyCaptainDanmu(const CaptainDanmuEvent& event)
{
    for (const auto& handler : captainDanmuListeners_)
    {
        handler(event);
    }
}