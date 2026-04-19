#pragma once
#include "framework.h"
#include "ProfileManager.h"
#include "TextToSpeech.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <regex>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

struct CaptainDanmuEvent {
    std::string uid;
    int32_t guardLevel = 0;
    bool hasMedal = false;
    std::string username;
    std::string content;
    int64_t serverTimestamp = 0;
    int32_t sendDate = 0;
};

struct CheckinEvent {
    std::string uid;
    std::string username;
    int32_t continuousDays = 0;
    int32_t checkinDate = 0;
    int32_t lastCheckinDate = 0;
};

struct AnswerResult {
    bool success = false;
    std::string answerContent;
    bool isAiGenerated = false;
    std::string errorMsg;
};

struct UserProfile {
    std::string uid;
    std::string username;
    std::vector<KeywordRecord> keywords;
    std::vector<std::pair<int64_t, std::string>> danmuHistory;
    int32_t lastCheckinDate = 0;
    int32_t continuousDays = 0;
    int64_t lastDanmuTimestamp = 0;
    int64_t createdAt = 0;
    int32_t sameContentCount = 0;
    std::string lastContent;
};

using AnswerCallback = std::function<void(const AnswerResult&)>;

class CaptainCheckInModule {
    DECLARE_SINGLETON(CaptainCheckInModule)

public:
    bool Init();
    void PushDanmuEvent(const CaptainDanmuEvent& event);
    void GenerateCheckinAnswerAsync(const CheckinEvent& event, AnswerCallback callback);
    AnswerResult GenerateCheckinAnswerSync(const CheckinEvent& event, UserProfile* profile = nullptr);
    const UserProfile* GetUserProfile(const std::string& uid) const;
    std::vector<std::string> GetUserTopKeywords(const std::string& uid) const;
    void SetTriggerWords(const std::string& words);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    bool IsCheckinMessage(const std::string& content) const;
    std::string GetFallbackAnswer(const CheckinEvent& event);

private:
    int32_t CalculateContinuousDays(const std::string& uid, int32_t checkinDate);
    std::string BuildPrompt(const CheckinEvent& event, const UserProfile* profile);
    std::string GenerateAIAnswer(const CheckinEvent& event, const UserProfile* profile);
    void ExtractKeywords(UserProfile& profile, const std::string& content);
    void LoadProfileFromDb(const std::string& uid);
    void SaveProfileToDb(const UserProfile& profile);
    void SaveProfileAsync(const UserProfile& profile);
    void LoadProfileAsync(const std::string& uid, std::function<void(UserProfile)> callback);

    bool ShouldLearn(const UserProfile& profile, const CaptainDanmuEvent& event);
    bool IsStopWord(const std::string& word) const;
    bool ShouldSkipDuplicateContent(UserProfile& profile, const std::string& content);
    int64_t GetCurrentTimestamp() const;
    std::string GetModuleDictPath() const;
    void PlayCheckinTTS(const std::string& text, const std::string& username);
    class JiebaContext;
    static JiebaContext* CreateJiebaContextSafe(const std::string& jiebaDict, const std::string& hmmModel, const std::string& userDict, const std::string& idfPath, const std::string& stopWords);

    std::map<std::string, UserProfile> profiles_;
    std::vector<std::wstring> triggerWords_;
    std::string triggerWordsStr_;
    mutable std::mutex profilesLock_;
    bool inited_ = false;
    bool enabled_ = true;

    int64_t lastLearnTimestamp_ = 0;

    std::string aiProviderJson_;

    class JiebaContext;
    std::unique_ptr<JiebaContext> jiebaContext_;
};