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
#include "cppjieba/Jieba.hpp"
#include <set>
#include <mutex>
#include <fstream>

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
    JiebaContext(const string& dictPath, const string& hmmModelPath, const string& userDictPath, const string& stopWordsPath)
        : jieba_(dictPath, hmmModelPath, userDictPath, stopWordsPath) {}

    cppjieba::Jieba jieba_;
};

CaptainCheckInModule* CaptainCheckInModule::__Instance = nullptr;

CaptainCheckInModule* CaptainCheckInModule::Inst() {
    if (!__Instance) __Instance = new CaptainCheckInModule();
    return __Instance;
}

void CaptainCheckInModule::Destroy() {
    LOG_INFO(TEXT("CaptainCheckInModule::Destroy"));

    lock_guard<std::mutex> lock(profilesLock_);
    for (auto& [uid, profile] : profiles_) {
        SaveProfileToDb(profile);
    }
    profiles_.clear();

    jiebaContext_.reset();
    triggerWordPatterns_.clear();
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

    auto& config = ConfigManager::Inst()->GetConfig();
    triggerWordsStr_ = config.checkinTriggerWords;

    std::wstringstream ss(std::wstring(config.checkinTriggerWords.begin(), config.checkinTriggerWords.end()));
    std::wstring word;
    while (std::getline(ss, word, L',')) {
        if (!word.empty()) {
            triggerWordPatterns_.push_back(std::wregex(word, std::wregex::icase));
        }
    }

    aiProviderJson_ = GetAI_PROVIDER();

    std::string dictPath = GetModuleDictPath();
    std::string jiebaDict = dictPath + "/jieba.dict.utf8";
    std::string hmmModel = dictPath + "/hmm_model.utf8";
    std::string userDict = "";
    std::string stopWords = dictPath + "/stop_words.utf8";

#if TEST_CAPTAIN_REPLY_LOCAL
    std::ifstream dictTest(jiebaDict);
    if (!dictTest.good()) {
        LOG_ERROR(TEXT("CaptainCheckInModule::Init ERROR: Dictionary file not found: %hs. Please ensure dict/jieba.dict.utf8 exists."), jiebaDict.c_str());
    }
    std::ifstream hmmTest(hmmModel);
    if (!hmmTest.good()) {
        LOG_ERROR(TEXT("CaptainCheckInModule::Init ERROR: Dictionary file not found: %hs. Please ensure dict/hmm_model.utf8 exists."), hmmModel.c_str());
    }
    std::ifstream stopTest(stopWords);
    if (!stopTest.good()) {
        LOG_ERROR(TEXT("CaptainCheckInModule::Init ERROR: Dictionary file not found: %hs. Please ensure dict/stop_words.utf8 exists."), stopWords.c_str());
    }
#endif

    try {
        jiebaContext_ = std::make_unique<JiebaContext>(jiebaDict, hmmModel, userDict, stopWords);
        LOG_INFO(TEXT("CaptainCheckInModule::Init cppjieba initialized successfully"));
    } catch (const std::exception& e) {
        LOG_ERROR(TEXT("CaptainCheckInModule::Init cppjieba init failed: %s, keyword extraction will be disabled"), e.what());
        jiebaContext_.reset();
    }

    inited_ = true;
    LOG_INFO(TEXT("CaptainCheckInModule::Init done, trigger words: %s"), triggerWordsStr_.c_str());

    DanmuProcessor::Inst()->AddCaptainDanmuListener([this](const DanmuProcessor::CaptainDanmuEvent& event) {
        if (IsEnabled()) {
            CaptainDanmuEvent convertedEvent;
            convertedEvent.uid = event.uid;
            convertedEvent.guardLevel = event.guardLevel;
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
    triggerWordPatterns_.clear();

    std::wstringstream ss(std::wstring(words.begin(), words.end()));
    std::wstring word;
    while (std::getline(ss, word, L',')) {
        if (!word.empty()) {
            triggerWordPatterns_.push_back(std::wregex(word, std::wregex::icase));
        }
    }
    LOG_INFO(TEXT("CaptainCheckInModule trigger words updated: %s"), words.c_str());
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
        std::wstring wcontent(content.begin(), content.end());
        for (const auto& pattern : triggerWordPatterns_) {
            if (std::regex_search(wcontent, pattern)) {
                return true;
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("IsCheckinMessage regex error: %s"), e.what());
    }
    return false;
}

bool CaptainCheckInModule::ShouldLearn(const UserProfile& profile, const CaptainDanmuEvent& event) {
#if TEST_CAPTAIN_REPLY_LOCAL
    return true;  // 跳过防刷屏检查，方便本地测试
#else
    int64_t now = GetCurrentTimestamp();

    if (now - profile.lastDanmuTimestamp < LEARN_TIME_WINDOW_MS) {
        return false;
    }

    if (ShouldSkipDuplicateContent(event.content)) {
        return false;
    }

    return true;
#endif
}

bool CaptainCheckInModule::ShouldSkipDuplicateContent(const std::string& content) {
    if (lastContent_.empty()) {
        lastContent_ = content;
        sameContentCount_ = 1;
        return false;
    }

    if (content == lastContent_) {
        sameContentCount_++;
        if (sameContentCount_ >= MAX_SAME_CONTENT_SKIP) {
            return true;
        }
    }
    else {
        sameContentCount_ = 1;
        lastContent_ = content;
    }
    return false;
}

void CaptainCheckInModule::PushDanmuEvent(const CaptainDanmuEvent& event) {
    if (!enabled_) return;

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

    if (!ShouldLearn(profile, event)) {
        return;
    }

    profile.lastDanmuTimestamp = event.serverTimestamp;

    profile.danmuHistory.push_back({event.serverTimestamp, event.content});
    if (profile.danmuHistory.size() > MAX_DANMU_HISTORY_SIZE) {
        profile.danmuHistory.erase(profile.danmuHistory.begin());
    }

    ExtractKeywords(profile, event.content);

    SaveProfileToDb(profile);

    if (IsCheckinMessage(event.content)) {
        int32_t checkinDate = event.sendDate;
        ProfileManager::Inst()->RecordCheckin(event.uid, event.username, checkinDate);
        int32_t continuousDays = ProfileManager::Inst()->CalculateContinuousDays(event.uid, checkinDate);

        CheckinEvent checkinEvt;
        checkinEvt.uid = event.uid;
        checkinEvt.username = event.username;
        checkinEvt.continuousDays = continuousDays;
        checkinEvt.checkinDate = checkinDate;

        AnswerResult result = GenerateCheckinAnswerSync(checkinEvt);
        if (result.success && !result.answerContent.empty()) {
            if (ConfigManager::Inst()->GetConfig().enableVoice) {
                PlayCheckinTTS(result.answerContent, event.username, event.serverTimestamp);
            }
        }
    }
}

void CaptainCheckInModule::ExtractKeywords(UserProfile& profile, const std::string& content) {
    if (!jiebaContext_) return;

    std::vector<std::string> words;
    jiebaContext_->jieba_.Cut(content, words, true);

    for (const auto& word : words) {
        if (word.length() < MIN_WORD_LENGTH) continue;
        if (IsStopWord(word)) continue;

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

bool CaptainCheckInModule::IsStopWord(const std::string& word) const {
    return STOP_WORDS.find(word) != STOP_WORDS.end();
}

void CaptainCheckInModule::LoadProfileFromDb(uint64_t uid) {
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

void CaptainCheckInModule::GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback) {
    AnswerResult result = GenerateCheckinAnswerSync(event);
    callback(result);
}

AnswerResult CaptainCheckInModule::GenerateCheckinAnswerSync(const CheckinEvent& event) {
    AnswerResult result;

    UserProfile* profile = nullptr;
    {
        lock_guard<std::mutex> lock(profilesLock_);
        auto it = profiles_.find(event.uid);
        if (it != profiles_.end()) {
            profile = &it->second;
        }
    }

    if (!aiProviderJson_.empty()) {
        auto provider = AIChatProviderFactory::Create(aiProviderJson_);
        if (provider && provider->IsAvailable()) {
            std::string prompt = BuildPrompt(event, profile);
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

    std::ostringstream oss;
    oss << "用户" << event.username << "是一位舰长，今日第" << event.continuousDays << "天打卡。\n"
        << "他的发言习惯包含：" << keywords << "\n"
        << "最近发言：" << recentMessages << "\n"
        << "请在回复中明确提到用户" << event.username << "的姓名，用轻松友好且有点皮的语气回复他的打卡，控制在20字以内。\n"
        << "回复内容需要适合TTS语音播报，避免生僻字和复杂句式。";
    return oss.str();
}

std::string CaptainCheckInModule::GetFallbackAnswer(const CheckinEvent& event) {
    if (event.continuousDays <= 1) {
        return event.username + "打卡成功！";
    }
    else {
        std::ostringstream oss;
        oss << event.username << "今日第" << event.continuousDays << "天打卡！";
        return oss.str();
    }
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

const UserProfile* CaptainCheckInModule::GetUserProfile(uint64_t uid) const {
    lock_guard<std::mutex> lock(profilesLock_);
    auto it = profiles_.find(uid);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> CaptainCheckInModule::GetUserTopKeywords(uint64_t uid) const {
    std::vector<std::string> result;
    auto profile = GetUserProfile(uid);
    if (!profile) return result;

    int count = (std::min)(10, (int)profile->keywords.size());
    for (int i = 0; i < count; ++i) {
        result.push_back(profile->keywords[i].word);
    }
    return result;
}

int32_t CaptainCheckInModule::CalculateContinuousDays(uint64_t uid, int32_t checkinDate) {
    return ProfileManager::Inst()->CalculateContinuousDays(uid, checkinDate);
}

int64_t CaptainCheckInModule::GetCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string CaptainCheckInModule::GetModuleDictPath() const {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);
    return exeDir + "/dict";
}

void CaptainCheckInModule::PlayCheckinTTS(const std::string& text, const std::string& username, int64_t timestamp) {
    if (aiProviderJson_.empty()) {
        LOG_WARNING(TEXT("PlayCheckinTTS: AI_PROVIDER is empty, cannot play TTS"));
        return;
    }

    auto ttsProvider = TTSProviderFactory::Create(aiProviderJson_);
    if (!ttsProvider || !ttsProvider->IsAvailable()) {
        LOG_WARNING(TEXT("PlayCheckinTTS: TTS provider not available"));
        return;
    }

    TTSRequest ttsReq;
    ttsReq.text = text;
    ttsReq.voice = "";
    ttsReq.style = "开心";
    ttsReq.speed = 1.0f;
    ttsReq.pitch = 0;
    ttsReq.volume = 1;

    ttsProvider->RequestTTS(ttsReq, [this, username, timestamp](const TTSResponse& response) {
        if (response.success && !response.audioData.empty()) {
            if (TTSCacheManager::Inst()->SaveCheckinAudio(username, response.audioData, timestamp)) {
                LOG_INFO(TEXT("PlayCheckinTTS: Saved checkin audio for %hs"), username.c_str());
            }
            TTSManager::Inst()->PlayAudioData(response.audioData, "mp3");
        } else {
            LOG_ERROR(TEXT("PlayCheckinTTS: TTS request failed: %hs"), response.errorMsg.c_str());
        }
    });
}