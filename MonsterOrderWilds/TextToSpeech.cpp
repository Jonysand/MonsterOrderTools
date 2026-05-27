#include "TextToSpeech.h"
#include "Network.h"
#include "ConfigManager.h"
#include "CredentialsManager.h"
#include "TTSProvider.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include "CaptainCheckInModule.h"
#include "RetroactiveCheckInModule.h"

#pragma warning(disable: 4996)

extern "C" {
typedef void(__stdcall* OnCheckinTTSPlayCallback)(const wchar_t* username, const wchar_t* content, void* userData);
extern OnCheckinTTSPlayCallback g_checkinTTSPlayCallback;
extern void* g_checkinTTSPlayUserData;
}
#include <sapi.h> // Include SAPI header for ISpVoice
#include <cstringt.h>
#include <sphelper.h>

DEFINE_SINGLETON(TTSManager)

#pragma comment(lib, "sapi.lib")

std::atomic<bool> TTSManager::s_instanceAlive{ false };

namespace {
    // SAPI event IDs (not all defined in Windows 10 SDK headers, using documented values)
    constexpr int SPEI_STREAM_ENDED_ID = 11;  // SPEI_STREAM_ENDED
}

TTSManager::TTSManager()
{
    s_instanceAlive = true;
    // Initialize COM library
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        s_instanceAlive = false;
        return; // Failed to initialize COM
    }
    hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    if (FAILED(hr)) {
        LOG_ERROR(TEXT("TTSManager: CoCreateInstance failed, hr=0x%08X"), hr);
        pVoice = NULL;
    }
    LastTickTime = std::chrono::steady_clock::now();
    lastFailureTime = std::chrono::steady_clock::now();
    lastRecoveryAttempt = std::chrono::steady_clock::now();


    const auto& config = ConfigManager::Inst()->GetConfig();
    userSelectedEngineName_ = config.ttsEngine;
    ttsProvider = TTSProviderFactory::Create(
        GetMIMO_API_KEY(),
        config.ttsEngine);
    audioPlayer = new AudioPlayer();
    TTSCacheManager::Inst()->Initialize();

    ConfigManager::Inst()->AddConfigChangedListener([this](const ConfigData& config) {
        this->RefreshTTSProvider();
    });

}

TTSManager::~TTSManager()
{
    ttsProvider.reset();
    if (audioPlayer != NULL) {
        delete audioPlayer;
        audioPlayer = NULL;
    }

    if (pVoice != NULL) {
        // 等待 SAPI 所有异步操作完成，防止回调在对象销毁后触发
        pVoice->WaitUntilDone(INFINITE);
        pVoice->Release();
        pVoice = NULL;
    }
    // 标记实例已销毁，回调检查到此标志后会安全退出
    s_instanceAlive = false;
    // Uninitialize COM library
    CoUninitialize();
}

void TTSManager::Tick()
{
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - LastTickTime).count();
    LastTickTime = now;

    if (ShouldTryRecovery()) {
        TryRecovery();
    }

    ProcessAsyncTTS();
    // 注意：ProcessAsyncTTS 已遍历整个队列并清理所有 Completed/Failed 请求，
    // 此处不需要额外的 CleanupCompletedRequests 调用

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (!NormalMsgQueue.empty())
        {
            LOG_INFO(TEXT("=== NormalMsgQueue has %zu messages ==="), NormalMsgQueue.size());
            if (ConfigManager::Inst()->GetConfig().enableVoice) {
                LOG_INFO(TEXT("=== enableVoice is true, calling Speak ==="));
                Speak(NormalMsgQueue.front());
            } else {
                LOG_INFO(TEXT("=== enableVoice is false, skipping Speak ==="));
            }
            NormalMsgQueue.pop_front();
        }
        if (!GiftMsgQueue.empty())
        {
            if (ConfigManager::Inst()->GetConfig().enableVoice)
                Speak(GiftMsgQueue.front());
            GiftMsgQueue.pop_front();
        }
        if (!HistoryLogMsgQueue.empty())
        {
            RECORD_HISTORY(HistoryLogMsgQueue.front().c_str());
            HistoryLogMsgQueue.pop_front();
        }

        for (auto it = ComboGiftMsgPrepareMap.begin(); it != ComboGiftMsgPrepareMap.end(); )
        {
            it->second.combo_timeout -= deltaTime;
            if (it->second.combo_timeout <= 0.0f)
            {
                TString msg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) + TEXT(" 赠送的") + std::to_wstring(it->second.gift_num) + TEXT("个") + utf8_to_wstring(it->second.gift_name);
                if (ConfigManager::Inst()->GetConfig().enableVoice && (!ConfigManager::Inst()->GetConfig().onlySpeekPaidGift || it->second.paid))
                    GiftMsgQueue.push_back(msg);
                HistoryLogMsgQueue.push_back(msg);
                it = ComboGiftMsgPrepareMap.erase(it);
            }
            else
                ++it;
        }

        for (auto it = dynamicComboMap_.begin(); it != dynamicComboMap_.end(); ) {
            it->second.combo_timeout -= deltaTime;
            if (it->second.combo_timeout <= 0.0f) {
                if (!it->second.firstReported || it->second.gift_num > 0) {
                    TString msg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) +
                                  TEXT(" 赠送的") + std::to_wstring(it->second.gift_num) +
                                  TEXT("个") + utf8_to_wstring(it->second.gift_name);
                    GiftMsgQueue.push_back(msg);
                    HistoryLogMsgQueue.push_back(msg);
                }
                it = dynamicComboMap_.erase(it);
            } else {
                ++it;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        static int64_t lastCooldownCleanup = 0;
        if (GetTickCount64() - lastCooldownCleanup > COOLDOWN_CLEANUP_INTERVAL_MS) {
            CleanupExpiredCooldowns();
            lastCooldownCleanup = GetTickCount64();
        }
    }
}

void TTSManager::HandleSpeekDm(const json& data)
{
    if (!data.contains("fans_medal_wearing_status") || !data.contains("guard_level") ||
        !data.contains("uname") || !data.contains("msg")) {
        return;
    }
    const auto& wearing_medal = data["fans_medal_wearing_status"].get<bool>();
    const auto& guard_level = data["guard_level"].get<int>();
    const auto& uname = data["uname"].get<std::string>();
    const auto& msg = utf8_to_wstring(data["msg"].get<std::string>());
    TString msgTString = utf8_to_wstring(uname) + TEXT(" 说：") + msg;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        HistoryLogMsgQueue.push_back(msgTString);
    }
    if (ConfigManager::Inst()->GetConfig().onlySpeekWearingMedal && !wearing_medal)
        return;
    if (ConfigManager::Inst()->GetConfig().onlySpeekGuardLevel != 0 && (guard_level == 0 || guard_level > ConfigManager::Inst()->GetConfig().onlySpeekGuardLevel))
        return;
    if (!ConfigManager::Inst()->GetConfig().enableVoice)
        return;
    if (msg.rfind(TEXT("点餐"), 0) == 0) {
        // 以"点餐"开头
        HandleDmOrderFood(msg, utf8_to_wstring(uname));
    }
    std::string msgUtf8 = WstringToUtf8(msg);
    if (CaptainCheckInModule::Inst()->IsCheckinMessage(msgUtf8)) {
        return;
    }
    if (RetroactiveCheckInModule::Inst()->IsRetroactiveMessage(msgUtf8) ||
        RetroactiveCheckInModule::Inst()->IsQueryMessage(msgUtf8)) {
        return;
    }
#if !ONLY_ORDER_MONSTER
    // 检查本地语音匹配（特殊语音不依赖TTS引擎选择）
    std::string voiceFile = LocalVoiceManager::Inst()->MatchVoice(msg);
    if (!voiceFile.empty()) {
        // 本地语音匹配成功，直接播放（不依赖TTS引擎选择）
        auto reqPtr = std::make_shared<AsyncTTSRequest>();
        reqPtr->text = msg;
        reqPtr->engineType = TTSEngineType::LocalVoice;
        reqPtr->state = AsyncTTSState::Pending;
        reqPtr->ResetTiming();
        reqPtr->responseFormat = "mp3";
        reqPtr->voice = voiceFile;

        {
            std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
            asyncPendingQueue_.push_back(reqPtr);
        }

        if (LocalVoiceManager::Inst()->IsSpecialVoice(msg)) {
            LOG_DEBUG(TEXT("HandleSpeekDm: Special local voice queued for: %s"), msg.c_str());
        } else {
            LOG_DEBUG(TEXT("HandleSpeekDm: Local voice queued for: %s"), msg.c_str());
        }
        return;
    }
#endif
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        NormalMsgQueue.push_back(msgTString);
    }
}

void TTSManager::HandleSpeekSendGift(const json& data)
{
    if (!data.contains("paid") || !data.contains("uname") || !data.contains("gift_name") ||
        !data.contains("gift_num") || !data.contains("open_id") || !data.contains("gift_id")) {
        return;
    }
    const auto& paid = data["paid"].get<bool>();
    const auto& uname = data["uname"].get<std::string>();
    const auto& gift_name = data["gift_name"].get<std::string>();
    int gift_num = data["gift_num"].get<int>();
    const auto& open_id = data["open_id"].get<std::string>();
    std::string gift_id = std::to_string(data["gift_id"].get<int>());
    std::string combo_id = open_id + gift_id;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        if (IsInCooldown(combo_id)) {
            auto it = dynamicComboMap_.find(combo_id);
            if (it != dynamicComboMap_.end()) {
                it->second.gift_num += gift_num;
                it->second.lastUpdateTime = GetTickCount64();
                it->second.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
            }
            return;
        }

        if (paid && data.contains("combo_info") && data["combo_info"].contains("combo_id") &&
            data["combo_info"].contains("combo_timeout") && data["combo_info"].contains("combo_base_num") &&
            data["combo_info"].contains("combo_count")) {
            std::string official_combo_id = data["combo_info"]["combo_id"].get<std::string>();
            int combo_timeout = data["combo_info"]["combo_timeout"].get<int>();
            int combo_base_num = data["combo_info"]["combo_base_num"].get<int>();
            int combo_count = data["combo_info"]["combo_count"].get<int>();
            gift_num = combo_base_num * combo_count;

            auto it = ComboGiftMsgPrepareMap.find(official_combo_id);
            if (it != ComboGiftMsgPrepareMap.end()) {
                it->second.combo_timeout = combo_timeout;
                it->second.gift_num = gift_num;
            } else {
                ComboGiftMsgEntry info;
                info.uname = uname;
                info.gift_name = gift_name;
                info.gift_num = gift_num;
                info.combo_timeout = combo_timeout;
                info.paid = paid;
                ComboGiftMsgPrepareMap.emplace(official_combo_id, std::move(info));
            }
        } else {
            auto it = dynamicComboMap_.find(combo_id);
            if (it != dynamicComboMap_.end()) {
                it->second.gift_num += gift_num;
                it->second.lastUpdateTime = GetTickCount64();
                it->second.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;

                if (!it->second.firstReported && it->second.gift_num >= 3) {
                    TString firstMsg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) +
                                       TEXT(" 开始赠送") + utf8_to_wstring(it->second.gift_name);
                    GiftMsgQueue.push_back(firstMsg);
                    HistoryLogMsgQueue.push_back(firstMsg);
                    it->second.firstReported = true;
                    UpdateCooldown(combo_id);
                }
            } else {
                DynamicComboEntry entry;
                entry.combo_id = combo_id;
                entry.uname = uname;
                entry.gift_name = gift_name;
                entry.gift_num = gift_num;
                entry.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
                entry.paid = paid;
                entry.firstReported = false;
                entry.lastUpdateTime = GetTickCount64();
                dynamicComboMap_.emplace(combo_id, std::move(entry));

                if (gift_num < 3) {
                    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 赠送的") +
                                  std::to_wstring(gift_num) + TEXT("个") + utf8_to_wstring(gift_name);
                    GiftMsgQueue.push_back(msg);
                    HistoryLogMsgQueue.push_back(msg);
                    UpdateCooldown(combo_id);

                    auto findIt = dynamicComboMap_.find(combo_id);
                    if (findIt != dynamicComboMap_.end()) {
                        findIt->second.firstReported = true;
                    }
                }
            }
        }
    }
}

void TTSManager::HandleSpeekSC(const json& data)
{
    if (!data.contains("uname") || !data.contains("rmb") || !data.contains("message")) {
        return;
    }
    const auto& uname = data["uname"].get<std::string>();
    const auto& rmb = data["rmb"].get<int>();
    const auto& message = data["message"].get<std::string>();
    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 赠送的") + std::to_wstring(rmb) + TEXT("元SC：") + utf8_to_wstring(message);
    std::lock_guard<std::mutex> lock(queueMutex_);
    HistoryLogMsgQueue.push_back(msg);
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekGuard(const json& data)
{
    if (!data.contains("user_info") || !data["user_info"].contains("uname") ||
        !data.contains("guard_level") || !data.contains("guard_num") || !data.contains("guard_unit")) {
        return;
    }
    const auto& uname = data["user_info"]["uname"].get<std::string>();
    const auto& guard_level = data["guard_level"].get<int>();
    const auto& guard_num = data["guard_num"].get<int>();
    const auto& guard_unit = data["guard_unit"].get<std::string>();

    TString guard_name;
    switch (guard_level)
    {
    case 1:
        guard_name = TEXT("总督");
        break;
    case 2:
        guard_name = TEXT("提督");
        break;
    case 3:
        guard_name = TEXT("舰长");
        break;
    default:
        LOG_ERROR(TEXT("Unknown guard level: %d"), guard_level);
        return;
    }
    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 上船") + std::to_wstring(guard_num) + utf8_to_wstring(guard_unit) + TEXT("的") + guard_name;
    std::lock_guard<std::mutex> lock(queueMutex_);
    HistoryLogMsgQueue.push_back(msg);
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekEnter(const json& data)
{
    if (!data.contains("uname")) {
        return;
    }
    const auto& uname = data["uname"].get<std::string>();
    TString msg = utf8_to_wstring(uname) + TEXT(" 进入直播间");
    std::lock_guard<std::mutex> lock(queueMutex_);
    HistoryLogMsgQueue.push_back(msg);
}

bool TTSManager::Speak(const TString& text)
{
    LOG_INFO(TEXT("=== TTS Speak called with text: %s ==="), text.c_str());

    auto reqPtr = std::make_shared<AsyncTTSRequest>();
    reqPtr->text = text;
    reqPtr->engineType = GetActiveEngineType();
    reqPtr->state = AsyncTTSState::Pending;
    reqPtr->ResetTiming();

    LOG_INFO(TEXT("TTS Engine type: %d"), (int)reqPtr->engineType);

    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    asyncPendingQueue_.push_back(reqPtr);
    return true;
}

bool TTSManager::PlayAudioData(const std::vector<uint8_t>& audioData, const std::string& format) {

    if (audioPlayer == NULL) {
        LOG_ERROR(TEXT("PlayAudioData: audioPlayer is NULL"));
        return false;
    }
    int speechVolume = ConfigManager::Inst()->GetConfig().speechVolume;
    audioPlayer->SetVolume(speechVolume);
    bool success = audioPlayer->Play(audioData, format);
    if (!success) {
        LOG_ERROR(TEXT("PlayAudioData: AudioPlayer::Play failed"));
    }
    return success;
}

void TTSManager::SpeakCheckinTTS(const TString& text, const std::string& username, std::function<void(bool success, const std::string& errorMsg)> callback) {

    auto reqPtr = std::make_shared<AsyncTTSRequest>();
    reqPtr->text = text;
    reqPtr->engineType = GetActiveEngineType();
    reqPtr->state = AsyncTTSState::Pending;
    reqPtr->ResetTiming();
    reqPtr->responseFormat = "mp3";
    reqPtr->isCheckinTTS = true;
    reqPtr->checkinUsername = username;
    reqPtr->callback = callback;

    {
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        asyncPendingQueue_.push_back(reqPtr);
    }
    LOG_DEBUG(TEXT("SpeakCheckinTTS: Request added to queue for: %s"), text.c_str());

}

void TTSManager::SetupSapiVoiceParams(ISpVoice* voice)
{
    ISpObjectToken* pChineseToken = NULL;
    HRESULT hrToken = SpFindBestToken(SPCAT_VOICES, L"Language=804", NULL, &pChineseToken);
    if (SUCCEEDED(hrToken) && pChineseToken) {
        voice->SetVoice(pChineseToken);
        LOG_DEBUG(TEXT("SetupSapiVoiceParams: Set Chinese voice"));
        pChineseToken->Release();
    } else {
        LOG_DEBUG(TEXT("SetupSapiVoiceParams: Chinese voice not found, hr=0x%08X"), hrToken);
    }

    int speechRate = ConfigManager::Inst()->GetConfig().speechRate;
    int speechVolume = ConfigManager::Inst()->GetConfig().speechVolume;
    int pitch = ConfigManager::Inst()->GetConfig().speechPitch;
    LOG_DEBUG(TEXT("SetupSapiVoiceParams: rate=%d, volume=%d, pitch=%d"), speechRate, speechVolume, pitch);
    voice->SetRate(speechRate);
    voice->SetVolume(speechVolume / 2);
}

std::wstring TTSManager::BuildSapiSsml(const TString& text)
{
    int pitch = ConfigManager::Inst()->GetConfig().speechPitch;
    std::wstring pitchStr = (pitch >= 0 ? L"+" : L"") + std::to_wstring(pitch) + L"st";

    std::wstring safeText;
    safeText.reserve(text.size() * 2);
    for (wchar_t ch : text) {
        if (ch == L'<') safeText += L"&lt;";
        else if (ch == L'&') safeText += L"&amp;";
        else if (ch == L'>') safeText += L"&gt;";
        else safeText += ch;
    }

    return L"<speak version='1.0' xml:lang='zh-CN'><prosody pitch='" + pitchStr + L"'>" + safeText + L"</prosody></speak>";
}

bool TTSManager::RecreateSapiVoice()
{
    // 注意：调用者需持有 sapiMutex_
    if (pVoice != NULL) {
        pVoice->WaitUntilDone(INFINITE);
        pVoice->Release();
        pVoice = NULL;
    }
    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    if (FAILED(hr)) {
        LOG_ERROR(TEXT("RecreateSapiVoice: CoCreateInstance failed, hr=0x%08X"), hr);
        pVoice = NULL;
        return false;
    }
    LOG_INFO(TEXT("RecreateSapiVoice: pVoice recreated successfully"));
    return true;
}

bool TTSManager::SpeakWithSapi(const TString& text)
{
    std::lock_guard<std::mutex> lock(sapiMutex_);

    LOG_DEBUG(TEXT("SpeakWithSapi: pVoice=%p"), pVoice);

    if (pVoice == NULL) {
        LOG_WARNING(TEXT("SpeakWithSapi: pVoice is NULL, attempting recreate"));
        if (!RecreateSapiVoice()) {
            return false;
        }
    }

    SetupSapiVoiceParams(pVoice);
    std::wstring ssml = BuildSapiSsml(text);
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL);
    LOG_DEBUG(TEXT("SpeakWithSapi: Speak result=0x%08X"), hr);

    if (FAILED(hr)) {
        LOG_WARNING(TEXT("SpeakWithSapi: Speak failed (hr=0x%08X), attempting recreate"), hr);
        if (RecreateSapiVoice()) {
            SetupSapiVoiceParams(pVoice);
            hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL);
            LOG_DEBUG(TEXT("SpeakWithSapi: Retry Speak result=0x%08X"), hr);
        }
    }

    return SUCCEEDED(hr);
}

TTSEngineType TTSManager::GetActiveEngineType() const
{
    // Bug #2 fix: When in fallback mode, always use SAPI
    if (isFallback) {
        return TTSEngineType::SAPI;
    }
    // API冷却期间直接走SAPI，避免重复尝试不可用的API
    auto now = std::chrono::steady_clock::now();
    if (now < apiCooldownExpiry) {
        return TTSEngineType::SAPI;
    }
    const auto& config = ConfigManager::Inst()->GetConfig();
    if (config.ttsEngine.empty() || config.ttsEngine == "auto") {
        return TTSEngineType::Auto;
    }
    if (config.ttsEngine == "manbo") {
        return TTSEngineType::Manbo;
    }
    if (config.ttsEngine == "mimo") {
        return TTSEngineType::MiMo;
    }
    if (config.ttsEngine == "sapi") {
        return TTSEngineType::SAPI;
    }
    return TTSEngineType::Auto;
}

bool TTSManager::SpeakWithSapiSync(const TString& text)
{
    std::lock_guard<std::mutex> lock(sapiMutex_);

    if (pVoice == NULL) {
        LOG_WARNING(TEXT("SpeakWithSapiSync: pVoice is NULL, attempting recreate"));
        if (!RecreateSapiVoice()) {
            return false;
        }
    }

    SetupSapiVoiceParams(pVoice);
    std::wstring ssml = BuildSapiSsml(text);
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML, NULL);
    LOG_DEBUG(TEXT("SpeakWithSapiSync: Speak result=0x%08X"), hr);

    if (FAILED(hr)) {
        LOG_WARNING(TEXT("SpeakWithSapiSync: Speak failed (hr=0x%08X), attempting recreate"), hr);
        if (RecreateSapiVoice()) {
            SetupSapiVoiceParams(pVoice);
            hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML, NULL);
            LOG_DEBUG(TEXT("SpeakWithSapiSync: Retry Speak result=0x%08X"), hr);
        }
    }

    return SUCCEEDED(hr);
}

void CALLBACK TTSManager::SapiSpeakCallback(WPARAM wParam, LPARAM lParam)
{
    // SAPI SetNotifyCallbackFunction 需要线程有消息泵才能正常触发回调
    // 此回调已弃用，改用 pVoice->GetStatus() 轮询检测播放完成
    // 保留空函数以避免编译错误
}

void TTSManager::RefreshTTSProvider()
{
    const auto& config = ConfigManager::Inst()->GetConfig();
    LOG_INFO(TEXT("Refreshing TTS provider, engine: %hs"), config.ttsEngine.c_str());
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    userSelectedEngineName_ = config.ttsEngine;
    isFallback = false;
    consecutiveFailures = 0;
    ttsProvider = TTSProviderFactory::Create(
        GetMIMO_API_KEY(),
        config.ttsEngine);
    LOG_INFO(TEXT("TTS provider refreshed successfully"));
}

std::string TTSManager::GetCurrentProviderName() const
{
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    if (ttsProvider) {
        return ttsProvider->GetProviderName();
    }
    return "none";
}


void TTSManager::SpeakWithMimoAsync(const TString& text, std::function<void(bool success, const std::string& errorMsg)> callback)
{
    LOG_INFO(TEXT("=== SpeakWithMimoAsync called ==="));
    
    if (!ttsProvider || audioPlayer == NULL) {
        LOG_ERROR(TEXT("SpeakWithMimoAsync: ttsProvider or audioPlayer is NULL, falling back to SAPI"));
        SpeakWithSapi(text);
        if (callback) {
            callback(false, "ttsProvider or audioPlayer is NULL");
        }
        return;
    }

    if (!ttsProvider->IsAvailable()) {
        LOG_WARNING(TEXT("MiMo TTS not available (API Key not configured), falling back to SAPI"));
        SpeakWithSapi(text);
        if (callback) {
            callback(false, "MiMo TTS not available");
        }
        return;
    }

    // 创建异步请求并加入等待队列
    auto reqPtr = std::make_shared<AsyncTTSRequest>();
    reqPtr->text = text;
    reqPtr->state = AsyncTTSState::Pending;
    reqPtr->ResetTiming();
    reqPtr->callback = callback;

    // 获取配置参数
    const auto& config = ConfigManager::Inst()->GetConfig();
    if (!config.mimoAudioFormat.empty()) {
        reqPtr->responseFormat = config.mimoAudioFormat;
    } else {
        reqPtr->responseFormat = "mp3";
    }

    // 需要加锁保护asyncPendingQueue_
    size_t queueSize;
    {
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        asyncPendingQueue_.push_back(reqPtr);
        queueSize = asyncPendingQueue_.size();
    }
    LOG_DEBUG(TEXT("SpeakWithMimoAsync: Request added to queue, queue size: %zu"), queueSize);
}

void TTSManager::ProcessAsyncTTS()
{
    // 处理所有当前活跃的请求（需要加锁保护asyncPendingQueue_）
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);

    // 处理活跃请求：遍历整个队列处理所有非 Pending 请求
    // Bug fix: 原代码用 processed < activeRequestCount_ 限制遍历数量，
    // 但 HandleRequestFailureInternal 重试时会减少 activeRequestCount_，
    // 导致后面的请求被跳过、卡在 Requesting 状态无法超时，造成队列积压。
    for (auto it = asyncPendingQueue_.begin(); it != asyncPendingQueue_.end(); ) {
        auto& reqPtr = *it;
        switch (reqPtr->state) {
        case AsyncTTSState::Pending:
            // Pending 请求由第二段循环处理（含重试间隔检查和并发限制）
            ++it;
            break;
        case AsyncTTSState::Requesting:
            ProcessRequestingStateInternal(*reqPtr);
            ++it;
            break;
        case AsyncTTSState::Playing:
            ProcessPlayingStateInternal(*reqPtr);
            ++it;
            break;
        case AsyncTTSState::Completed:
        case AsyncTTSState::Failed:
            LOG_INFO(TEXT("TTS Async: Request %s, cleaning up"),
                reqPtr->state == AsyncTTSState::Completed ? TEXT("completed") : TEXT("failed"));
            it = asyncPendingQueue_.erase(it);
            if (activeRequestCount_ > 0) {
                activeRequestCount_--;
            } else {
                LOG_WARNING(TEXT("TTS Async: activeRequestCount_ already 0, skipping decrement"));
            }
            break;
        }
    }

	// 从队列取新的请求（遍历队列找 Pending，不只是检查队首）
	// SAPI 引擎已有串行控制，不受 MAX_CONCURRENT_TTS 限制；MiMo 引擎保留并发限制
	while (!asyncPendingQueue_.empty()) {
		bool startedAny = false;
		for (auto it = asyncPendingQueue_.begin(); it != asyncPendingQueue_.end(); ++it) {
			if ((*it)->state == AsyncTTSState::Pending) {
				// 检查重试间隔：如果请求正在等待重试间隔，跳过
				if ((*it)->retryCount > 0 && (*it)->retryAfterTime != std::chrono::steady_clock::time_point()) {
					auto now = std::chrono::steady_clock::now();
					if (now < (*it)->retryAfterTime) {
						auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>((*it)->retryAfterTime - now).count();
						LOG_DEBUG(TEXT("TTS Async: Request waiting retry interval (%lld ms), skipping: %s"), waitMs, (*it)->text.c_str());
						continue; // 跳过这个请求，继续查找下一个 Pending
					}
				}
				// 判断是否使用 SAPI 引擎
				auto now = std::chrono::steady_clock::now();
				bool willUseSapi = ((*it)->engineType == TTSEngineType::SAPI) ||
					isFallback ||
					(now < apiCooldownExpiry) ||
					((*it)->hasTriedSapiFallback) ||
					(ttsProvider && ttsProvider->GetProviderName() == "sapi");
				if (willUseSapi) {
					// SAPI 请求需要串行播放：检查是否已有 SAPI 请求正在播放
					bool hasSapiPlaying = false;
					for (auto& req : asyncPendingQueue_) {
						if (req->engineType == TTSEngineType::SAPI && req->state == AsyncTTSState::Playing) {
							hasSapiPlaying = true;
							break;
						}
					}
					if (hasSapiPlaying) {
						LOG_DEBUG(TEXT("TTS Async: SAPI request queued, waiting for previous to complete: %s"),
							(*it)->text.c_str());
						continue; // 跳过这个SAPI请求，继续查找下一个非SAPI的Pending请求
					}
					// SAPI 引擎：不受 MAX_CONCURRENT_TTS 限制，由串行控制保证顺序
				} else {
					// MiMo 引擎：检查并发上限
					if (activeRequestCount_ >= MAX_CONCURRENT_TTS) {
						LOG_DEBUG(TEXT("TTS Async: MiMo concurrent limit reached (%d), skipping"), activeRequestCount_.load());
						continue;
					}
				}

				(*it)->requestStartTime = std::chrono::steady_clock::now();
				activeRequestCount_++;
				LOG_INFO(TEXT("TTS Async: Starting new request for: %s (active: %d)"),
					(*it)->text.c_str(), activeRequestCount_.load());
				ProcessPendingRequestInternal(it);
				startedAny = true;
				break; // 启动了一个请求后退出内层循环，让外层while检查并发上限
			}
		}
		if (!startedAny) {
			break; // 没有启动任何请求（所有Pending都被跳过或没有Pending），退出外层循环
		}
	}
}

void TTSManager::ProcessPendingRequestInternal(std::list<std::shared_ptr<AsyncTTSRequest>>::iterator it)
{
    auto& reqPtr = *it;
    AsyncTTSRequest& req = *reqPtr;

    // 处理本地音频请求
    if (req.engineType == TTSEngineType::LocalVoice) {
        LOG_INFO(TEXT("TTS Async: Processing LocalVoice request"));
        std::vector<uint8_t> audioData;
        if (!req.voice.empty() && LocalVoiceManager::Inst()->LoadVoiceData(
                req.voice, audioData)) {
            req.audioData = std::move(audioData);
            req.state = AsyncTTSState::Playing;
            LOG_INFO(TEXT("TTS Async: Local voice loaded, state -> Playing"));
        } else {
            req.state = AsyncTTSState::Failed;
            req.errorMessage = "Failed to load local voice";
            LOG_ERROR(TEXT("TTS Async: Failed to load local voice for: %s"), req.text.c_str());
            if (req.callback) {
                req.callback(false, req.errorMessage);
            }
        }
        return;
    }

    // 处理 SAPI 请求（异步播放 + SPEVENT 回调）
    // 当当前 Provider 为 SAPI 时（如降级后），强制走 SAPI 分支，避免同步回调覆盖状态导致重复播报
    const bool isSapiProvider = ttsProvider && ttsProvider->GetProviderName() == "sapi";
    if (req.engineType == TTSEngineType::SAPI || isSapiProvider) {
        if (req.engineType != TTSEngineType::SAPI) {
            req.engineType = TTSEngineType::SAPI;
            LOG_INFO(TEXT("TTS Async: Provider is SAPI, routing request to SAPI branch"));
        }
        LOG_INFO(TEXT("TTS Async: Processing SAPI request"));
        if (req.isCheckinTTS && !req.checkinUsername.empty()) {
            std::wstring usernameW = Utf8ToWstring(req.checkinUsername);
            std::wstring contentW = req.text;
            if (g_checkinTTSPlayCallback) {
                try {
                    g_checkinTTSPlayCallback(usernameW.c_str(), contentW.c_str(), g_checkinTTSPlayUserData);
                } catch (...) {
                    LOG_ERROR(TEXT("CheckinTTSPlayCallback threw exception"));
                }
            }
        }
        req.state = AsyncTTSState::Playing;
        req.playbackStarted = true;
        req.playbackStartTime = std::chrono::steady_clock::now();
        req.sapiStreamEnded = false;

        {
            std::lock_guard<std::mutex> lock(sapiMutex_);
            if (pVoice == NULL) {
                LOG_WARNING(TEXT("TTS Async: pVoice is NULL, attempting recreate"));
                if (!RecreateSapiVoice()) {
                    LOG_ERROR(TEXT("TTS Async: RecreateSapiVoice failed, SAPI playback failed"));
                    req.state = AsyncTTSState::Failed;
                    req.errorMessage = "pVoice is NULL and recreate failed";
                    if (req.callback) {
                        req.callback(false, req.errorMessage);
                    }
                    return;
                }
            }

            SetupSapiVoiceParams(pVoice);
            std::wstring ssml = BuildSapiSsml(req.text);
            HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL);
            if (SUCCEEDED(hr)) {
                LOG_DEBUG(TEXT("TTS Async: SAPI Speak started with SPF_ASYNC"));
                if (req.callback) {
                    req.callback(true, "");
                }
            }
            else {
                LOG_ERROR(TEXT("TTS Async: SAPI Speak failed, hr=0x%08X"), hr);
                req.state = AsyncTTSState::Failed;
                req.errorMessage = "SAPI speak failed";
                if (req.callback) {
                    req.callback(false, req.errorMessage);
                }
            }
        }
        return;
    }

    // 处理 MiMo 请求
    // Pending → Requesting: 发起API请求
    if (req.text.empty()) {
        LOG_ERROR(TEXT("TTS Async: req.text is empty!"));
        req.state = AsyncTTSState::Failed;
        req.errorMessage = "req.text is empty";
        if (req.callback) {
            req.callback(false, req.errorMessage);
        }
        return;
    }

    if (!ttsProvider) {
        req.state = AsyncTTSState::Failed;
        req.errorMessage = "ttsProvider is NULL";
        if (req.callback) {
            req.callback(false, req.errorMessage);
        }
        return;
    }

    // 构建请求参数
    TTSRequest ttsReq;

    // 获取配置参数
    const auto& config = ConfigManager::Inst()->GetConfig();

    // voice 由各 Provider 自己从 ConfigManager 读取

    ttsReq.text = wstring_to_utf8(req.text);

    LOG_INFO(TEXT("TTS Async: Sending API request for: %s"), req.text.c_str());

    // 发起异步请求（回调在HTTP线程执行）
    // 注意：捕获shared_ptr而不是迭代器，shared_ptr在回调执行期间始终有效
    // 因为即使请求从队列中被移除，只要lambda还持有shared_ptr，内存就不会释放
    // 注意：这里不需要获取asyncMutex_，因为调用此函数的ProcessAsyncTTS已经持有锁
    // HTTP回调会在不同线程执行，回调内部会获取锁
    if (asyncPendingQueue_.empty()) {
        LOG_ERROR(TEXT("TTS Async: Request queue is empty, cannot send request"));
        return;
    }
    ttsProvider->RequestTTS(ttsReq, [this, reqPtr](const TTSResponse& response) {
        // 回调在HTTP线程中执行，需要线程安全地修改状态
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        // reqPtr 是 shared_ptr，只要lambda还持有它，请求就不会被释放
        // 即使请求已从队列中移除，只要回调还在执行，shared_ptr就有效
        if (response.success && !response.audioData.empty()) {
            reqPtr->audioData = response.audioData;
            reqPtr->state = AsyncTTSState::Playing;
            LOG_INFO(TEXT("TTS Async: API request succeeded, starting playback"));

            if (reqPtr->isCheckinTTS && !reqPtr->checkinUsername.empty()) {
                TTSCacheManager::Inst()->SaveCheckinAudio(reqPtr->checkinUsername, response.audioData, GetTickCount64());
            }

            if (reqPtr->callback) {
                reqPtr->callback(true, "");
            }
        } else {
            // 如果请求已转到SAPI fallback，忽略迟到的API响应
            if (reqPtr->hasTriedSapiFallback && reqPtr->engineType == TTSEngineType::SAPI) {
                LOG_INFO(TEXT("TTS Async: Ignoring late API response, request already switched to SAPI"));
                return;
            }
            reqPtr->errorMessage = response.errorMsg;
            LOG_ERROR(TEXT("TTS Async: API request failed (HTTP %d): %s"), response.httpStatusCode, utf8_to_wstring(response.errorMsg).c_str());
            if (HandleRequestFailureInternal(*reqPtr) && reqPtr->callback) {
                reqPtr->callback(false, response.errorMsg);
            }
        }
    });

    req.state = AsyncTTSState::Requesting;
    req.requestStartTime = std::chrono::steady_clock::now();
}

void TTSManager::ProcessRequestingStateInternal(AsyncTTSRequest& req)
{
    // 注意：此函数在ProcessAsyncTTS持有asyncMutex_时被调用，不要再获取锁

    if (req.state != AsyncTTSState::Requesting) {
        return;
    }

    // 如果已经有音频数据，说明回调已执行，状态改为Playing让下次Tick处理播放
    if (!req.audioData.empty()) {
        req.state = AsyncTTSState::Playing;
        LOG_INFO(TEXT("TTS Async: State changed to Playing"));
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto totalElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.totalStartTime).count();

    // 总时间上限检查：防止重试后无限等待
    if (totalElapsed >= MAX_TOTAL_TIMEOUT_SECONDS) {
        LOG_WARNING(TEXT("TTS Async: Total request timeout (%lld seconds), forcing failure"), (long long)totalElapsed);
        if (req.state == AsyncTTSState::Requesting && req.audioData.empty()) {
            // 尝试SAPI fallback：设置冷却并转SAPI重新排队
            if (!req.hasTriedSapiFallback) {
                req.hasTriedSapiFallback = true;
                req.engineType = TTSEngineType::SAPI;
                req.state = AsyncTTSState::Pending;
                req.retryCount = 0;
                req.requestStartTime = std::chrono::steady_clock::now();
                req.totalStartTime = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                apiCooldownExpiry = now + std::chrono::seconds(API_COOLDOWN_SECONDS);
                isFallback = true;
                if (activeRequestCount_ > 0) activeRequestCount_--;
                LOG_WARNING(TEXT("TTS Async: Total timeout, switching to SAPI fallback (cooldown %ds)"), API_COOLDOWN_SECONDS);
                return;
            }
            // 已试过SAPI仍然失败，标记真正失败
            req.state = AsyncTTSState::Failed;
            req.errorMessage = "Total timeout exceeded";
            consecutiveFailures = MAX_CONSECUTIVE_FAILURES;
            lastFailureTime = now;
            TriggerFallback();
            LOG_ERROR(TEXT("TTS Async: Request failed after total timeout (%llds), even with SAPI fallback"), (long long)totalElapsed);
            if (req.callback) {
                req.callback(false, req.errorMessage);
            }
        }
        return;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.requestStartTime).count();

    if (elapsed >= API_TIMEOUT_SECONDS) {
        LOG_WARNING(TEXT("TTS Async: API request timeout (%lld seconds)"), (long long)elapsed);
        // 不在这里标记失败，因为HTTP回调可能还在处理中
        // 等到下次Tick，如果audioData有数据会转为Playing，没有才会真正失败
        // 为了避免无限等待，最多等待1个额外的超时周期
        if (elapsed >= API_TIMEOUT_SECONDS * 2) {
            // Race condition guard: double-check state and audioData before handling failure
            // HTTP callback might have just completed between the audioData check above and now
            if (req.state == AsyncTTSState::Requesting && req.audioData.empty()) {
                HandleRequestFailureInternal(req);
            } else {
                LOG_INFO(TEXT("TTS Async: Request state changed or audioData arrived during timeout check, skipping failure handling"));
            }
        }
    }
}

void TTSManager::ProcessPlayingStateInternal(AsyncTTSRequest& req)
{
    // 注意：此函数在ProcessAsyncTTS持有asyncMutex_时被调用，不要再获取锁

    // 处理 SAPI 请求完成：使用 pVoice->GetStatus() 轮询检测（SetNotifyCallbackFunction 需要消息泵）
    if (req.engineType == TTSEngineType::SAPI) {
        if (req.sapiStreamEnded) {
            LOG_INFO(TEXT("TTS Async: SAPI playback completed (SPEI_STREAM_ENDED)"));
            req.state = AsyncTTSState::Completed;
            consecutiveFailures = 0;
            lastFailureTime = std::chrono::steady_clock::now();
            return;
        }

        // 轮询 pVoice->GetStatus() 检测播放完成
        if (pVoice) {
            SPVOICESTATUS status;
            HRESULT hr = pVoice->GetStatus(&status, NULL);
            if (SUCCEEDED(hr) && status.dwRunningState == SPRS_DONE) {
                LOG_INFO(TEXT("TTS Async: SAPI playback completed (GetStatus: SPRS_DONE)"));
                req.state = AsyncTTSState::Completed;
                consecutiveFailures = 0;
                lastFailureTime = std::chrono::steady_clock::now();
                return;
            }
        }

        // SAPI 播放超时检查：防止永久占用并发槽
        if (SAPI_PLAYBACK_TIMEOUT_SECONDS > 0 && req.playbackStartTime != std::chrono::steady_clock::time_point()) {
            auto now = std::chrono::steady_clock::now();
            auto playbackElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.playbackStartTime).count();
            if (playbackElapsed > SAPI_PLAYBACK_TIMEOUT_SECONDS) {
                LOG_WARNING(TEXT("TTS Async: SAPI playback timeout (%lld seconds), treating as completed"), (long long)playbackElapsed);
                req.state = AsyncTTSState::Completed;
                consecutiveFailures = 0;
                lastFailureTime = std::chrono::steady_clock::now();
            }
        }
        return;
    }

    if (audioPlayer == NULL) {
        req.state = AsyncTTSState::Failed;
        req.errorMessage = "audioPlayer is NULL";
        return;
    }

    // 播放音频（非阻塞模式）
    if (!req.audioData.empty() && !audioPlayer->IsPlaying() && !req.playbackStarted) {
        int speechVolume = ConfigManager::Inst()->GetConfig().speechVolume;
        audioPlayer->SetVolume(speechVolume);
        bool playSuccess = audioPlayer->Play(req.audioData, req.responseFormat);
        if (!playSuccess) {
            LOG_ERROR(TEXT("TTS Async: Audio playback failed"));
            req.state = AsyncTTSState::Failed;
            return;
        }
        req.playbackStarted = true;
        req.audioData.clear();
        req.playbackStartTime = std::chrono::steady_clock::now();  // 重置播放超时计时
        LOG_INFO(TEXT("TTS Async: Audio playback started"));

        if (req.isCheckinTTS && !req.checkinUsername.empty()) {
            std::wstring usernameW = Utf8ToWstring(req.checkinUsername);
            std::wstring contentW = req.text;
            if (g_checkinTTSPlayCallback) {
                try {
                    g_checkinTTSPlayCallback(usernameW.c_str(), contentW.c_str(), g_checkinTTSPlayUserData);
                } catch (...) {
                    LOG_ERROR(TEXT("CheckinTTSPlayCallback threw exception"));
                }
            }
        }
    }

    // 检查播放完成：只有真正开始播放了才检查完成状态
    if (req.playbackStarted) {
        bool playbackComplete = audioPlayer->IsPlaybackComplete();
        
        bool timedOut = false;
        if (PLAYBACK_TIMEOUT_SECONDS > 0) {
            auto now = std::chrono::steady_clock::now();
            auto playbackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.playbackStartTime).count();
            timedOut = (playbackElapsed > PLAYBACK_TIMEOUT_SECONDS * 1000);
        }
        
        if (playbackComplete || timedOut) {
            if (playbackComplete) {
                LOG_INFO(TEXT("TTS Async: Playback completed (MCI reported stop)"));
            } else {
                LOG_WARNING(TEXT("TTS Async: Playback timeout (%d ms), treating as completed"), 
                    (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - req.playbackStartTime).count());
                if (audioPlayer != NULL) {
                    audioPlayer->Stop();
                }
            }
            if (audioPlayer != NULL) {
                audioPlayer->CleanupTempFile();
            }
            req.state = AsyncTTSState::Completed;
            consecutiveFailures = 0;
            lastFailureTime = std::chrono::steady_clock::now();
            return;
        }
    }
}

bool TTSManager::TrySwitchToNextProvider()
{
    if (!ttsProvider) return false;

    // 手动选择引擎时不永久切换 provider，由 TryRecovery 在下次请求时自动恢复
    if (IsManualEngineMode()) {
        LOG_INFO(TEXT("TTS: Manual engine mode (%hs), skipping provider switch"), userSelectedEngineName_.c_str());
        return false;
    }

    std::string currentName = ttsProvider->GetProviderName();
    std::string nextEngine;

    // 降级链：manbo -> mimo -> sapi
    if (currentName == "manbo") {
        if (!GetMIMO_API_KEY().empty()) nextEngine = "mimo";
        else nextEngine = "sapi";
    } else if (currentName == "xiaomi") {
        nextEngine = "sapi";
    } else {
        return false; // 已经是SAPI或未知Provider，无法降级
    }

    LOG_WARNING(TEXT("TTS: Downgrading from %hs to %hs"), currentName.c_str(), nextEngine.c_str());
    ttsProvider = TTSProviderFactory::Create(GetMIMO_API_KEY(), nextEngine);

    if (ttsProvider) {
        LOG_INFO(TEXT("TTS: Successfully switched to %hs"), ttsProvider->GetProviderName().c_str());
        return true;
    }

    return false;
}

bool TTSManager::HandleRequestFailureInternal(AsyncTTSRequest& req)
{
    // 注意：此函数在ProcessAsyncTTS持有asyncMutex_时被调用，不要再获取锁
    // 返回值：true = 请求真正失败（不再重试），false = 正在重试/SAPI fallback

    // Race condition guard: only handle failure if request is still in an expected state
    if (req.state != AsyncTTSState::Requesting && req.state != AsyncTTSState::Playing) {
        LOG_DEBUG(TEXT("TTS Async: HandleRequestFailureInternal called but request state is %d, not Requesting/Playing. Skipping."), (int)req.state);
        return false;
    }

    // If audioData has arrived (callback completed just before we got the lock), don't fail
    if (!req.audioData.empty()) {
        LOG_INFO(TEXT("TTS Async: audioData arrived before failure handling, converting to Playing state"));
        req.state = AsyncTTSState::Playing;
        return false;
    }

    // API失败 → 立即尝试SAPI fallback（一次失败即切换，不重试API）
    if (!req.hasTriedSapiFallback) {
        req.hasTriedSapiFallback = true;
        req.engineType = TTSEngineType::SAPI;
        req.state = AsyncTTSState::Pending;
        req.retryCount = 0;
        req.requestStartTime = std::chrono::steady_clock::now();
        req.totalStartTime = std::chrono::steady_clock::now();
        req.errorMessage.clear();
        auto now = std::chrono::steady_clock::now();
        apiCooldownExpiry = now + std::chrono::seconds(API_COOLDOWN_SECONDS);
        isFallback = true;
        if (activeRequestCount_ > 0) activeRequestCount_--;
        LOG_WARNING(TEXT("TTS Async: API failed, switching to SAPI fallback (cooldown %ds)"), API_COOLDOWN_SECONDS);
        return false;
    }

    // 已试过SAPI仍然失败，标记真正失败
    req.state = AsyncTTSState::Failed;
    LOG_ERROR(TEXT("TTS Async: Request failed even with SAPI fallback"));
    consecutiveFailures++;
    lastFailureTime = std::chrono::steady_clock::now();
    return true;
}

bool TTSManager::IsUsingMimoTTS() const
{

    TTSEngine engine = GetActiveEngine();
    return (engine == TTSEngine::MIMO || (engine == TTSEngine::AUTO && !isFallback));
}

void TTSManager::RefreshEngineStatus()
{

    isFallback = false;
    consecutiveFailures = 0;
    apiCooldownExpiry = std::chrono::steady_clock::time_point();
    LOG_INFO(TEXT("TTS engine status refreshed"));

}

TTSManager::TTSEngine TTSManager::GetActiveEngine() const
{

    const auto& config = ConfigManager::Inst()->GetConfig();
    if (config.ttsEngine.empty()) {
        return TTSEngine::AUTO;
    }
    
    if (config.ttsEngine == "mimo") {
        return TTSEngine::MIMO;
    } else if (config.ttsEngine == "sapi") {
        return TTSEngine::SAPI;
    } else {
        return TTSEngine::AUTO;
    }
}

void TTSManager::TriggerFallback()
{

    if (!isFallback) {
        isFallback = true;
        apiCooldownExpiry = std::chrono::steady_clock::now() + std::chrono::seconds(API_COOLDOWN_SECONDS);
        fallbackReason = "Consecutive failures exceeded limit";
        LOG_WARNING(TEXT("TTS engine fallback triggered: switching to SAPI (cooldown %ds)"), API_COOLDOWN_SECONDS);
    }

}

void TTSManager::TryRecovery()
{

    if (!ttsProvider) {
        return;
    }

    LOG_INFO(TEXT("Attempting TTS recovery..."));

    // 确定目标引擎：手动模式用用户选择，自动模式默认 manbo
    std::string targetEngine;
    if (IsManualEngineMode()) {
        targetEngine = userSelectedEngineName_;
    } else {
        targetEngine = "manbo";
    }

    // 如果当前 provider 与目标引擎不同，先恢复原始引擎
    std::string currentName = ttsProvider->GetProviderName();
    std::string targetProviderName;
    if (targetEngine == "manbo") targetProviderName = "manbo";
    else if (targetEngine == "mimo") targetProviderName = "xiaomi";
    else if (targetEngine == "sapi") targetProviderName = "sapi";

    if (!targetProviderName.empty() && currentName != targetProviderName) {
        LOG_INFO(TEXT("TTS recovery: restoring engine %hs (current: %hs)"),
            targetEngine.c_str(), currentName.c_str());
        auto restored = TTSProviderFactory::Create(GetMIMO_API_KEY(), targetEngine);
        if (restored) {
            ttsProvider = restored;
        } else {
            LOG_WARNING(TEXT("TTS recovery: failed to create engine %hs"), targetEngine.c_str());
        }
    }

    // 冷却到期，清除fallback状态，让下一个真实请求验证API是否恢复
    isFallback = false;
    consecutiveFailures = 0;
    lastRecoveryAttempt = std::chrono::steady_clock::now();
    LOG_INFO(TEXT("TTS recovery: cooldown expired, cleared fallback. Next request will test API."));

}

bool TTSManager::ShouldTryRecovery() const
{

    if (!isFallback) {
        return false;
    }

    // 冷却期间不尝试恢复，等冷却到期
    auto now = std::chrono::steady_clock::now();
    if (now < apiCooldownExpiry) {
        return false;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRecoveryAttempt).count();
    return elapsed >= RECOVERY_INTERVAL_SECONDS;
}

bool TTSManager::IsInCooldown(const std::string& comboId) {
    auto it = giftCooldownMap_.find(comboId);
    if (it == giftCooldownMap_.end()) return false;
    
    int64_t cooldownMs = GIFT_COOLDOWN_SECONDS * 1000;
    return (GetTickCount64() - it->second) < cooldownMs;
}

void TTSManager::UpdateCooldown(const std::string& comboId) {
    giftCooldownMap_[comboId] = GetTickCount64();
}

void TTSManager::CleanupExpiredCooldowns() {
    int64_t cooldownMs = GIFT_COOLDOWN_SECONDS * 1000;
    int64_t now = GetTickCount64();
    for (auto it = giftCooldownMap_.begin(); it != giftCooldownMap_.end(); ) {
        if (now - it->second > cooldownMs * 2) {
            it = giftCooldownMap_.erase(it);
        } else ++it;
    }
}

void TTSManager::HandleDmOrderFood(const std::wstring& msg, const std::wstring& uname)
{
    if (msg.length() <= 2) return;
    std::wstring msgWithoutPrefix = msg.substr(2);
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 60);
    int randomValue = dist(rng);
    std::lock_guard<std::mutex> lock(queueMutex_);
    NormalMsgQueue.push_back(uname + TEXT(" 下单的 ") + msgWithoutPrefix + TEXT(" 已接单，预计") + std::to_wstring(randomValue) + TEXT("分钟后送达！"));
}
