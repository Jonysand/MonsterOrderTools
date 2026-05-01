#include "framework.h"
#include "CaptainCheckInModule.h"
#include "CredentialsManager.h"
#include "ConfigManager.h"
#include "ProfileManager.h"
#include "AIChatProvider.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "TextToSpeech.h"
#include "TTSProvider.h"
#include "TTSCacheManager.h"
#include "DanmuProcessor.h"
#include "DataBridgeExports.h"
#include "WriteQueue.h"
#include "cppjieba/Jieba.hpp"
#include <set>
#include <mutex>
#include <fstream>
#include <eh.h>
#include <psapi.h>

#ifdef RUN_UNIT_TESTS
#include <iostream>
#endif

using namespace std;

namespace {
    constexpr size_t MAX_KEYWORDS_COUNT = 50;
    constexpr size_t MAX_DANMU_HISTORY_SIZE = 100;
    constexpr int32_t MIN_WORD_LENGTH = 2;
    constexpr int64_t LEARN_TIME_WINDOW_MS = 5000;
    constexpr int32_t MAX_SAME_CONTENT_SKIP = 3;

    bool IsLeapYear(int32_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int32_t GetDaysInMonth(int32_t year, int32_t month) {
        switch (month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                return 31;
            case 4: case 6: case 9: case 11:
                return 30;
            case 2:
                return IsLeapYear(year) ? 29 : 28;
            default:
                return 30;
        }
    }

    const std::set<std::string> STOP_WORDS = {
        "的", "了", "在", "是", "我", "你", "他", "她", "它",
        "这", "那", "都", "和", "与", "或", "一", "一下",
        "吗", "呢", "吧", "啊", "哦", "嗯", "哈哈", "嘿嘿",
        "可以", "什么", "怎么", "为什么", "有没有", "但是",
        "然后", "所以", "因为", "如果", "虽然", "但是"
    };
}

class CaptainCheckInModule::JiebaContext {
public:
    JiebaContext(const string& dictPath, const string& hmmModelPath, const string& userDictPath, const string& idfPath, const string& stopWordsPath)
        : jieba_(dictPath, hmmModelPath, userDictPath, idfPath, stopWordsPath) {}

    cppjieba::Jieba jieba_;
};

CaptainCheckInModule::JiebaContext* CaptainCheckInModule::CreateJiebaContextSafe(
    const std::string& jiebaDict,
    const std::string& hmmModel,
    const std::string& userDict,
    const std::string& idfPath,
    const std::string& stopWords
) {
    try {
        return new (std::nothrow) JiebaContext(
            jiebaDict, hmmModel, userDict, idfPath, stopWords);
    } catch (...) {
        LOG_ERROR(TEXT("CreateJiebaContextSafe: caught exception during JiebaContext construction"));
        return nullptr;
    }
}

CaptainCheckInModule* CaptainCheckInModule::__Instance = nullptr;

CaptainCheckInModule* CaptainCheckInModule::Inst() {
    if (!__Instance) __Instance = new CaptainCheckInModule();
    return __Instance;
}

void CaptainCheckInModule::Destroy() {
    LOG_INFO(TEXT("CaptainCheckInModule::Destroy"));

    {
        lock_guard<std::mutex> lock(profilesLock_);
        for (auto& [uid, profile] : profiles_) {
            SaveProfileToDb(profile);
        }
        profiles_.clear();
    }

    jiebaContext_.reset();
    triggerWords_.clear();
    inited_ = false;
    enabled_ = true;

    LOG_INFO(TEXT("CaptainCheckInModule::Destroy done"));

    if (__Instance) {
        delete __Instance;
        __Instance = nullptr;
    }
}

bool CaptainCheckInModule::Init() {
    if (inited_) return true;

    LOG_INFO(TEXT("CaptainCheckInModule::Init"));

    const auto config = ConfigManager::Inst()->GetConfig();
    SetTriggerWords(config.checkinTriggerWords);

    aiProviderJson_ = GetAI_PROVIDER();

    std::string dictPath = GetModuleDictPath();
    std::string jiebaDict = dictPath + "\\jieba.dict.utf8";
    std::string hmmModel = dictPath + "\\hmm_model.utf8";
    std::string userDict = dictPath + "\\user.dict.utf8";
    std::string idfPath = dictPath + "\\idf.utf8";
    std::string stopWords = dictPath + "\\stop_words.utf8";

    JiebaContext* rawContext = CreateJiebaContextSafe(jiebaDict, hmmModel, userDict, idfPath, stopWords);
    jiebaContext_.reset(rawContext);
    
    if (!jiebaContext_) {
        LOG_ERROR(TEXT("CaptainCheckInModule::Init: JiebaContext creation failed, keyword extraction disabled"));
    }

    LoadStopWords(stopWords);

    inited_ = true;
    LOG_INFO(TEXT("CaptainCheckInModule::Init done"));

    DanmuProcessor::Inst()->AddCaptainDanmuListener([this](const DanmuProcessor::CaptainDanmuEvent& event) {
        if (IsEnabled()) {
            CaptainDanmuEvent convertedEvent;
            convertedEvent.uid = event.uid;
            convertedEvent.guardLevel = event.guardLevel;
            convertedEvent.hasMedal = event.hasMedal;
            convertedEvent.username = event.username;
            convertedEvent.content = event.content;
            convertedEvent.serverTimestamp = event.serverTimestamp;
            convertedEvent.sendDate = event.sendDate;
            PushDanmuEvent(convertedEvent);
        }
    });

    return true;
}

void CaptainCheckInModule::SetTriggerWords(const std::string& words) {
    triggerWordsStr_ = words;
    triggerWords_.clear();

    std::wstring wwords = Utf8ToWstring(words);
    std::wstring word;
    for (size_t i = 0; i <= wwords.length(); ++i) {
        wchar_t c = (i < wwords.length()) ? wwords[i] : L'\0';
        // 支持中英文逗号作为分隔符（英文逗号 U+002C，中文逗号 U+FF0C）
        if (c == L',' || c == L'\uFF0C' || c == L'\0') {
            if (!word.empty()) {
                // Trim leading and trailing spaces
                size_t start = word.find_first_not_of(L" \t\r\n");
                if (start != std::wstring::npos) {
                    size_t end = word.find_last_not_of(L" \t\r\n");
                    word = word.substr(start, end - start + 1);
                } else {
                    word.clear();
                }
                if (!word.empty()) {
                    triggerWords_.push_back(word);
                }
                word.clear();
            }
        } else {
            word += c;
        }
    }
    LOG_INFO(TEXT("CaptainCheckInModule trigger words updated: %hs"), words.c_str());
}

void CaptainCheckInModule::SetEnabled(bool enabled) {
    enabled_ = enabled;
    LOG_INFO(TEXT("CaptainCheckInModule enabled: %s"), enabled ? TEXT("true") : TEXT("false"));
}

bool CaptainCheckInModule::IsEnabled() const {
    return enabled_;
}

bool CaptainCheckInModule::IsCheckinMessage(const std::string& content) const {
    if (!enabled_) return false;

    try {
        std::wstring wcontent = Utf8ToWstring(content);
        for (const auto& word : triggerWords_) {
            if (_wcsicmp(wcontent.c_str(), word.c_str()) == 0) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsCheckinMessage error: %hs"), e.what());
    }
    return false;
}

bool CaptainCheckInModule::ShouldLearn(const UserProfile& profile, const CaptainDanmuEvent& event) {
#if TEST_CAPTAIN_REPLY_LOCAL
    return true;  // 跳过防刷屏检查，方便本地测试
#else
    if (event.guardLevel <= 0) {
        return false;
    }

    int64_t now = GetCurrentTimestamp();

    if (now - profile.lastDanmuTimestamp < LEARN_TIME_WINDOW_MS) {
        return false;
    }

    return true;
#endif
}

bool CaptainCheckInModule::ShouldSkipDuplicateContent(UserProfile& profile, const std::string& content) {
    if (profile.lastContent.empty()) {
        profile.lastContent = content;
        profile.sameContentCount = 1;
        return false;
    }

    if (content == profile.lastContent) {
        profile.sameContentCount++;
        if (profile.sameContentCount >= MAX_SAME_CONTENT_SKIP) {
            return true;
        }
    }
    else {
        profile.sameContentCount = 1;
        profile.lastContent = content;
    }
    return false;
}

void CaptainCheckInModule::PushDanmuEvent(const CaptainDanmuEvent& event) {
    if (!enabled_) return;

    LOG_DEBUG(TEXT("[CaptainCheckInModule] PushDanmuEvent: username=%hs, uid=%hs, guardLevel=%d, hasMedal=%d, content=%hs"),
        event.username.c_str(), event.uid.c_str(), event.guardLevel, event.hasMedal, event.content.c_str());

    bool shouldCheckin = IsCheckinMessage(event.content);

    LOG_DEBUG(TEXT("[CaptainCheckInModule] shouldCheckin=%d"), shouldCheckin);

    {
        lock_guard<std::mutex> lock(profilesLock_);

        auto it = profiles_.find(event.uid);
        if (it == profiles_.end()) {
            LoadProfileFromDb(event.uid);
            it = profiles_.find(event.uid);
            if (it == profiles_.end()) {
                UserProfile newProfile;
                newProfile.uid = event.uid;
                newProfile.username = event.username;
                newProfile.createdAt = GetCurrentTimestamp();
                profiles_[event.uid] = std::move(newProfile);
                it = profiles_.find(event.uid);
            }
        }

        UserProfile& profile = it->second;
        profile.username = event.username;

        if (ShouldLearn(profile, event)) {
            profile.lastDanmuTimestamp = event.serverTimestamp;

            profile.danmuHistory.push_back({event.serverTimestamp, event.content});
            if (profile.danmuHistory.size() > MAX_DANMU_HISTORY_SIZE) {
                profile.danmuHistory.erase(profile.danmuHistory.begin());
            }

            ExtractKeywords(profile, event.content);
        }

#if !TEST_CAPTAIN_REPLY_LOCAL
        if (ShouldSkipDuplicateContent(profile, event.content)) {
            return;
        }
#endif

        if (shouldCheckin && profile.lastCheckinDate == event.sendDate) {
            LOG_DEBUG(TEXT("[CaptainCheckInModule] Repeated checkin detected for uid=%hs, date=%d"), event.uid.c_str(), event.sendDate);
            int32_t continuousDays = ProfileManager::Inst()->CalculateContinuousDays(event.uid, event.sendDate);
            int32_t cumulativeDays = profile.cumulativeDays > 0 ? profile.cumulativeDays : continuousDays;
            std::string repeatedAnswer = event.username + "今日已打卡，连续" + std::to_string(continuousDays) + "天，累计" + std::to_string(cumulativeDays) + "天";

            std::wstring contentCopy = Utf8ToWstring(repeatedAnswer);
            RECORD_HISTORY(contentCopy.c_str());

            if (!ConfigManager::Inst()->GetConfig().enableVoice) {
                std::wstring usernameW = Utf8ToWstring(event.username);
                std::wstring answerW = Utf8ToWstring(repeatedAnswer);
                if (g_aiReplyCallback) {
                    g_aiReplyCallback(usernameW.c_str(), answerW.c_str(), g_aiReplyUserData);
                }
            } else {
                PlayCheckinTTS(repeatedAnswer, event.username);
            }

            SaveProfileAsync(profile);
            return;
        }

        if (shouldCheckin) {
            LOG_DEBUG(TEXT("[CaptainCheckInModule] Processing checkin for uid=%hs, guardLevel=%d, date=%d"), event.uid.c_str(), event.guardLevel, event.sendDate);
            int32_t checkinDate = event.sendDate;
            int32_t continuousDays = ProfileManager::Inst()->CalculateContinuousDays(event.uid, checkinDate);
            int32_t previousCheckinDate = profile.lastCheckinDate;
            int32_t cumulativeDays = ProfileManager::Inst()->CalculateCumulativeDays(event.uid, checkinDate);

            // 检查是否已在处理首次打卡中（含超时清理）
            bool alreadyPending = false;
            {
                std::lock_guard<std::mutex> lock(pendingCheckinLock_);
                auto it = pendingCheckinData_.find(event.uid);
                if (it != pendingCheckinData_.end()) {
                    int64_t elapsed = GetCurrentTimestamp() - it->second.startTime;
                    if (elapsed > PENDING_CHECKIN_TIMEOUT_MS) {
                        LOG_WARNING(TEXT("[CaptainCheckInModule] Pending checkin timed out for uid=%hs, clearing and retrying"), event.uid.c_str());
                        pendingCheckinData_.erase(it);
                    } else {
                        alreadyPending = true;
                    }
                }
            }

            if (alreadyPending) {
                LOG_DEBUG(TEXT("[CaptainCheckInModule] Checkin already pending for uid=%hs, ignoring duplicate"), event.uid.c_str());
                return;
            }

            // 首次打卡：延迟保存到数据库，直到AI回复和TTS成功
            {
                std::lock_guard<std::mutex> lock(pendingCheckinLock_);
                pendingCheckinData_[event.uid] = {event.username, checkinDate, continuousDays, cumulativeDays, GetCurrentTimestamp()};
            }

            CheckinEvent checkinEvt;
            checkinEvt.uid = event.uid;
            checkinEvt.username = event.username;
            checkinEvt.continuousDays = continuousDays;
            checkinEvt.cumulativeDays = cumulativeDays;
            checkinEvt.checkinDate = checkinDate;
            checkinEvt.lastCheckinDate = previousCheckinDate;

            auto saveCheckinRecord = [this, uid = event.uid](bool success) {
                PendingCheckinData data;
                bool found = false;
                {
                    std::lock_guard<std::mutex> lock(pendingCheckinLock_);
                    auto it = pendingCheckinData_.find(uid);
                    if (it != pendingCheckinData_.end()) {
                        data = it->second;
                        found = true;
                        pendingCheckinData_.erase(it);
                    }
                }

                if (found && success) {
                    {
                        std::lock_guard<std::mutex> profileLock(profilesLock_);
                        auto it = profiles_.find(uid);
                        if (it != profiles_.end()) {
                            it->second.lastCheckinDate = data.checkinDate;
                            it->second.continuousDays = data.continuousDays;
                            it->second.cumulativeDays = data.cumulativeDays;
                            SaveProfileAsync(it->second);
                        }
                    }
                    WriteQueue::Inst()->Enqueue(WriteTask{
                        .type = WriteTaskType::CHECKIN,
                        .uid = uid,
                        .username = data.username,
                        .execute = [uid, username = data.username, checkinDate = data.checkinDate, continuousDays = data.continuousDays, cumulativeDays = data.cumulativeDays](ProfileManager* pm) -> bool {
                            return pm->RecordCheckinSync(uid, username, checkinDate, continuousDays, cumulativeDays);
                        }
                    });
                    LOG_INFO(TEXT("[CaptainCheckInModule] Checkin record saved after TTS success for uid=%hs"), uid.c_str());
                } else if (found && !success) {
                    LOG_WARNING(TEXT("[CaptainCheckInModule] Checkin TTS/AI failed for uid=%hs, record not saved"), uid.c_str());
                }
            };

            if (event.guardLevel > 0) {
                GenerateCheckinAnswerAsync(checkinEvt, [this, event, saveCheckinRecord](const AnswerResult& result) {
                    if (result.success && !result.answerContent.empty()) {
                        std::wstring contentCopy = Utf8ToWstring(result.answerContent);
                        RECORD_HISTORY(contentCopy.c_str());

                        if (!ConfigManager::Inst()->GetConfig().enableVoice) {
                            std::wstring usernameW = Utf8ToWstring(event.username);
                            std::wstring answerW = Utf8ToWstring(result.answerContent);
                            if (g_aiReplyCallback) {
                                g_aiReplyCallback(usernameW.c_str(), answerW.c_str(), g_aiReplyUserData);
                            }
                            saveCheckinRecord(true);
                        }

                        if (ConfigManager::Inst()->GetConfig().enableVoice) {
                            PlayCheckinTTS(result.answerContent, event.username, saveCheckinRecord);
                        }
                    } else {
                        saveCheckinRecord(false);
                    }
                });
            } else {
                std::string fixedAnswer = GetFallbackAnswer(checkinEvt);
                std::wstring contentCopy = Utf8ToWstring(fixedAnswer);
                RECORD_HISTORY(contentCopy.c_str());

                if (!ConfigManager::Inst()->GetConfig().enableVoice) {
                    std::wstring usernameW = Utf8ToWstring(event.username);
                    std::wstring answerW = Utf8ToWstring(fixedAnswer);
                    if (g_aiReplyCallback) {
                        g_aiReplyCallback(usernameW.c_str(), answerW.c_str(), g_aiReplyUserData);
                    }
                    saveCheckinRecord(true);
                } else {
                    PlayCheckinTTS(fixedAnswer, event.username, saveCheckinRecord);
                }
            }
        } else {
            SaveProfileAsync(profile);
        }
    }
}

void CaptainCheckInModule::ExtractKeywords(UserProfile& profile, const std::string& content) {
    if (!jiebaContext_) return;

    // 提取 #标签# 中的标签词，学习时排除
    std::set<std::string> hashtagLabels;
    std::regex hashtagPattern(R"(#([^#]+)#)");
    std::sregex_iterator iter(content.begin(), content.end(), hashtagPattern);
    std::sregex_iterator end;
    for (; iter != end; ++iter) {
        std::string label = (*iter)[1].str();
        std::vector<std::string> labelWords;
        jiebaContext_->jieba_.Cut(label, labelWords, true);
        for (const auto& w : labelWords) {
            if (w.length() >= MIN_WORD_LENGTH) {
                hashtagLabels.insert(w);
            }
        }
    }

    std::vector<std::string> words;
    jiebaContext_->jieba_.Cut(content, words, true);

    for (const auto& word : words) {
        if (word.length() < MIN_WORD_LENGTH) continue;
        if (IsStopWord(word)) continue;
        if (hashtagLabels.find(word) != hashtagLabels.end()) continue;

        auto it = std::find_if(profile.keywords.begin(), profile.keywords.end(),
            [&word](const KeywordRecord& r) { return r.word == word; });

        if (it != profile.keywords.end()) {
            it->frequency++;
            it->lastSeenTimestamp = GetCurrentTimestamp();
        }
        else {
            if (profile.keywords.size() >= MAX_KEYWORDS_COUNT) {
                auto minIt = std::min_element(profile.keywords.begin(), profile.keywords.end(),
                    [](const KeywordRecord& a, const KeywordRecord& b) { return a.frequency < b.frequency; });
                profile.keywords.erase(minIt);
            }
            profile.keywords.push_back({0, word, 1, GetCurrentTimestamp()});
        }
    }

    std::sort(profile.keywords.begin(), profile.keywords.end(),
        [](const KeywordRecord& a, const KeywordRecord& b) { return a.frequency > b.frequency; });
}

void CaptainCheckInModule::LoadStopWords(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR(TEXT("CaptainCheckInModule::LoadStopWords: failed to open %hs"), filePath.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t' || line[start] == '\r' || line[start] == '\n')) {
            ++start;
        }
        size_t end = line.size();
        while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t' || line[end - 1] == '\r' || line[end - 1] == '\n')) {
            --end;
        }
        std::string word = line.substr(start, end - start);
        if (!word.empty()) {
            stopWords_.insert(word);
        }
    }

    LOG_INFO(TEXT("CaptainCheckInModule::LoadStopWords: loaded %zu words from %hs"), stopWords_.size(), filePath.c_str());
}

bool CaptainCheckInModule::IsStopWord(const std::string& word) const {
    if (!stopWords_.empty()) {
        return stopWords_.find(word) != stopWords_.end();
    }
    return STOP_WORDS.find(word) != STOP_WORDS.end();
}

void CaptainCheckInModule::LoadProfileFromDb(const std::string& uid) {
    UserProfileData dbProfile;
    if (ProfileManager::Inst()->LoadProfile(uid, dbProfile)) {
        auto it = profiles_.find(uid);
        if (it == profiles_.end()) {
            UserProfile profile;
            profile.uid = dbProfile.uid;
            profile.username = dbProfile.username;
            profile.lastCheckinDate = dbProfile.lastCheckinDate;
            profile.continuousDays = dbProfile.continuousDays;
            profile.lastDanmuTimestamp = dbProfile.lastDanmuTimestamp;
            profile.createdAt = dbProfile.createdAt;
            profile.keywords = dbProfile.keywords;
            profile.danmuHistory = dbProfile.danmuHistory;
            profiles_[uid] = std::move(profile);
        } else {
            it->second.username = dbProfile.username;
            it->second.lastCheckinDate = dbProfile.lastCheckinDate;
            it->second.continuousDays = dbProfile.continuousDays;
            it->second.lastDanmuTimestamp = dbProfile.lastDanmuTimestamp;
            it->second.keywords = dbProfile.keywords;
            it->second.danmuHistory = dbProfile.danmuHistory;
        }
    }
}

void CaptainCheckInModule::SaveProfileToDb(const UserProfile& profile) {
    UserProfileData dbProfile;
    dbProfile.uid = profile.uid;
    dbProfile.username = profile.username;
    dbProfile.lastCheckinDate = profile.lastCheckinDate;
    dbProfile.continuousDays = profile.continuousDays;
    dbProfile.lastDanmuTimestamp = profile.lastDanmuTimestamp;
    dbProfile.createdAt = profile.createdAt;
    dbProfile.keywords = profile.keywords;
    dbProfile.danmuHistory = profile.danmuHistory;
    ProfileManager::Inst()->SaveProfile(dbProfile);
}

void CaptainCheckInModule::SaveProfileAsync(const UserProfile& profile) {
    UserProfile profileCopy = profile;
    std::thread([profileCopy]() {
        UserProfileData dbProfile;
        dbProfile.uid = profileCopy.uid;
        dbProfile.username = profileCopy.username;
        dbProfile.lastCheckinDate = profileCopy.lastCheckinDate;
        dbProfile.continuousDays = profileCopy.continuousDays;
        dbProfile.lastDanmuTimestamp = profileCopy.lastDanmuTimestamp;
        dbProfile.createdAt = profileCopy.createdAt;
        dbProfile.keywords = profileCopy.keywords;
        dbProfile.danmuHistory = profileCopy.danmuHistory;
        ProfileManager::Inst()->SaveProfile(dbProfile);
    }).detach();
}

void CaptainCheckInModule::LoadProfileAsync(const std::string& uid, std::function<void(UserProfile)> callback) {
    std::thread([this, uid, callback]() {
        UserProfileData dbProfile;
        if (ProfileManager::Inst()->LoadProfile(uid, dbProfile)) {
            UserProfile profile;
            profile.uid = dbProfile.uid;
            profile.username = dbProfile.username;
            profile.lastCheckinDate = dbProfile.lastCheckinDate;
            profile.continuousDays = dbProfile.continuousDays;
            profile.lastDanmuTimestamp = dbProfile.lastDanmuTimestamp;
            profile.createdAt = dbProfile.createdAt;
            profile.keywords = dbProfile.keywords;
            profile.danmuHistory = dbProfile.danmuHistory;
            
            lock_guard<std::mutex> lock(profilesLock_);
            profiles_[uid] = profile;
            
            if (callback) {
                callback(profile);
            }
        }
    }).detach();
}

void CaptainCheckInModule::GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback) {
    CheckinEvent evtCopy = event;
    auto resultCb = [callback](const AnswerResult& result) {
        if (callback) callback(result);
    };
    
    std::thread([this, evtCopy, resultCb]() {
        AnswerResult result = GenerateCheckinAnswerSync(evtCopy, nullptr);
        resultCb(result);
    }).detach();
}

AnswerResult CaptainCheckInModule::GenerateCheckinAnswerSync(const CheckinEvent& event, UserProfile* profile) {
    AnswerResult result;

    UserProfile profileCopy;
    const UserProfile* profilePtr = profile;

    if (!profilePtr) {
        lock_guard<std::mutex> lock(profilesLock_);
        auto it = profiles_.find(event.uid);
        if (it != profiles_.end()) {
            profileCopy = it->second;
            profilePtr = &profileCopy;
        }
    }

    if (!aiProviderJson_.empty()) {
        auto provider = AIChatProviderFactory::Create(aiProviderJson_);
        if (provider && provider->IsAvailable()) {
            std::string prompt = BuildPrompt(event, profilePtr);
            std::string aiResponse;
            if (provider->CallAPI(prompt, aiResponse)) {
                result.success = true;
                result.answerContent = aiResponse;
                result.isAiGenerated = true;
                return result;
            }
        }
    }

    result.success = true;
    result.answerContent = GetFallbackAnswer(event);
    result.isAiGenerated = false;
    LOG_DEBUG(TEXT("[CaptainCheckInModule] Using fallback answer=%hs"), result.answerContent.c_str());
    return result;
}

std::string CaptainCheckInModule::BuildPrompt(const CheckinEvent& event, const UserProfile* profile) {
    std::string keywords;
    if (profile && !profile->keywords.empty()) {
        int count = (std::min)(5, (int)profile->keywords.size());
        for (int i = 0; i < count; ++i) {
            if (i > 0) keywords += "、";
            keywords += profile->keywords[i].word;
        }
    }
    else {
        keywords = "（暂无发言习惯数据）";
    }

    std::string recentMessages;
    if (profile && !profile->danmuHistory.empty()) {
        int count = (std::min)(3, (int)profile->danmuHistory.size());
        for (int i = 0; i < count; ++i) {
            if (i > 0) recentMessages += "；";
            recentMessages += profile->danmuHistory[profile->danmuHistory.size() - 1 - i].second;
        }
    }
    else {
        recentMessages = "（暂无历史发言）";
    }

    std::string lastCheckinInfo;
    if (event.lastCheckinDate > 0) {
        int32_t lastYear = event.lastCheckinDate / 10000;
        int32_t lastMonth = (event.lastCheckinDate % 10000) / 100;
        int32_t lastDay = event.lastCheckinDate % 100;
        int32_t daysSinceLastCheckin = 0;
        if (event.checkinDate > event.lastCheckinDate) {
            int32_t currentYear = event.checkinDate / 10000;
            int32_t currentMonth = (event.checkinDate % 10000) / 100;
            int32_t currentDay = event.checkinDate % 100;
            if (currentYear == lastYear && currentMonth == lastMonth) {
                daysSinceLastCheckin = currentDay - lastDay;
            } else if (currentYear == lastYear && currentMonth == lastMonth + 1) {
                int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
                if (currentDay == 1) {
                    daysSinceLastCheckin = lastMonthDays - lastDay + 1;
                }
            } else if (currentMonth == 1 && lastMonth == 12 && currentYear == lastYear + 1) {
                if (currentDay == 1) {
                    daysSinceLastCheckin = 31 - lastDay + 1;
                }
            } else {
                daysSinceLastCheckin = currentDay;
            }
        }
        std::ostringstream infoOss;
        infoOss << "，上次打卡是" << lastMonth << "月" << lastDay << "日";
        if (daysSinceLastCheckin > 0) {
            infoOss << "（" << daysSinceLastCheckin << "天前）";
        }
        lastCheckinInfo = infoOss.str();
    }

    std::ostringstream oss;
    oss << "用户" << event.username << "是一位舰长，连续第" << event.continuousDays << "天打卡，累计打卡" << event.cumulativeDays << "天" << lastCheckinInfo << "。\n"
        << "他的发言习惯包含：" << keywords << "\n"
        << "最近发言：" << recentMessages << "\n"
        << "请在回复中明确提到用户" << event.username << "的姓名，用轻松友好且有点皮的语气回复他的打卡，控制在20字以内。\n"
        << "回复内容需要适合TTS语音播报，避免生僻字和复杂句式。";
    return oss.str();
}

std::string CaptainCheckInModule::GetFallbackAnswer(const CheckinEvent& event) {
    std::ostringstream oss;
    oss << event.username << "连续第" << event.continuousDays << "天打卡！累计" << event.cumulativeDays << "天";
    return oss.str();
}

std::string CaptainCheckInModule::GenerateAIAnswer(const CheckinEvent& event, const UserProfile* profile) {
    std::string prompt = BuildPrompt(event, profile);
    std::string response;

    auto provider = AIChatProviderFactory::Create(aiProviderJson_);
    if (provider && provider->CallAPI(prompt, response)) {
        return response;
    }
    return GetFallbackAnswer(event);
}

bool CaptainCheckInModule::GetUserProfile(const std::string& uid, UserProfile& outProfile) const {
    lock_guard<std::mutex> lock(profilesLock_);
    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        outProfile = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> CaptainCheckInModule::GetUserTopKeywords(const std::string& uid) const {
    std::vector<std::string> result;
    UserProfile profile;
    if (!GetUserProfile(uid, profile)) return result;

    int count = (std::min)(10, (int)profile.keywords.size());
    for (int i = 0; i < count; ++i) {
        result.push_back(profile.keywords[i].word);
    }
    return result;
}

int32_t CaptainCheckInModule::CalculateContinuousDays(const std::string& uid, int32_t checkinDate) {
    return ProfileManager::Inst()->CalculateContinuousDays(uid, checkinDate);
}

int64_t CaptainCheckInModule::GetCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string CaptainCheckInModule::GetModuleDictPath() const {
    wchar_t exePathW[MAX_PATH];
    GetModuleFileNameW(NULL, exePathW, MAX_PATH);
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, exePathW, -1, nullptr, 0, nullptr, nullptr);
    std::string exePath(utf8Len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, exePathW, -1, &exePath[0], utf8Len, nullptr, nullptr);
    if (!exePath.empty() && exePath.back() == '\0') {
        exePath.pop_back();
    }
    std::string exeDir = exePath;
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);
    return exeDir + "/dict";
}

void CaptainCheckInModule::PlayCheckinTTS(const std::string& text, const std::string& username, std::function<void(bool success)> callback) {
    if (!ConfigManager::Inst()->GetConfig().enableVoice) {
        if (callback) {
            callback(true);
        }
        return;
    }

    TString ttsText = Utf8ToWstring(text);
    TTSManager::Inst()->SpeakCheckinTTS(ttsText, username, [callback](bool success, const std::string& errorMsg) {
        if (callback) {
            callback(success);
        }
    });
}
