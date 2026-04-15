#include "TextToSpeech.h"
#include "Network.h"
#include "ConfigManager.h"
#include "CredentialsManager.h"
#include "TTSProvider.h"
#include "WriteLog.h"
#include "StringUtils.h"

#pragma warning(disable: 4996)
#include <sapi.h> // Include SAPI header for ISpVoice
#include <cstringt.h>
#include <sphelper.h>

DEFINE_SINGLETON(TTSManager)

#pragma comment(lib, "sapi.lib")
TTSManager::TTSManager()
{
    // Initialize COM library
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        return; // Failed to initialize COM
    }
    hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    LastTickTime = std::chrono::steady_clock::now();
    lastFailureTime = std::chrono::steady_clock::now();
    lastRecoveryAttempt = std::chrono::steady_clock::now();

#if USE_MIMO_TTS
    const auto& config = ConfigManager::Inst()->GetConfig();
    ttsProvider = TTSProviderFactory::Create(
        GetMIMO_API_KEY(),
        GetMINIMAX_API_KEY(),
        config.ttsEngine);
    audioPlayer = new AudioPlayer();
    TTSCacheManager::Inst()->Initialize();
#endif
}

TTSManager::~TTSManager()
{
#if USE_MIMO_TTS
    ttsProvider.reset();
    if (audioPlayer != NULL) {
        delete audioPlayer;
        audioPlayer = NULL;
    }
#endif

    if (pVoice != NULL) {
        pVoice->Release();
        pVoice = NULL;
    }
    // Uninitialize COM library
    CoUninitialize();
}

void TTSManager::Tick()
{
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - LastTickTime).count();
    LastTickTime = now;

    // 检查是否需要尝试恢复MiMo TTS
#if USE_MIMO_TTS
    if (ShouldTryRecovery()) {
        TryRecovery();
    }

    // 处理异步TTS状态机
    ProcessAsyncTTS();
    CleanupCompletedRequests();
#endif

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
    
#if USE_MIMO_TTS
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
    
    static int64_t lastCooldownCleanup = 0;
    if (GetTickCount64() - lastCooldownCleanup > 60000) {
        CleanupExpiredCooldowns();
        lastCooldownCleanup = GetTickCount64();
    }
#endif
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
    HistoryLogMsgQueue.push_back(msgTString);
    if (ConfigManager::Inst()->GetConfig().onlySpeekWearingMedal && !wearing_medal)
        return;
    if (ConfigManager::Inst()->GetConfig().onlySpeekGuardLevel != 0 && (guard_level == 0 || guard_level > ConfigManager::Inst()->GetConfig().onlySpeekGuardLevel))
        return;
    if (msg.rfind(TEXT("点餐"), 0) == 0) {
        // 以"点餐"开头
        HandleDmOrderFood(msg, utf8_to_wstring(uname));
    }
    else if (msg == TEXT("签到") || msg == TEXT("打卡")) {
    }
    else
        NormalMsgQueue.push_back(msgTString);
}

void TTSManager::HandleSpeekSendGift(const json& data)
{
    const auto& paid = data["paid"].get<bool>();
    const auto& uname = data["uname"].get<std::string>();
    const auto& gift_name = data["gift_name"].get<std::string>();
    int gift_num = data["gift_num"].get<int>();
    const auto& open_id = data["open_id"].get<std::string>();
    std::string gift_id = std::to_string(data["gift_id"].get<int>());
    std::string combo_id = open_id + gift_id;

    if (IsInCooldown(combo_id)) {
        auto it = dynamicComboMap_.find(combo_id);
        if (it != dynamicComboMap_.end()) {
            it->second.gift_num += gift_num;
            it->second.lastUpdateTime = GetTickCount64();
            it->second.combo_timeout = DYNAMIC_COMBO_WINDOW_SECONDS;
        }
        return;
    }

    if (paid && data.contains("combo_info")) {
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

void TTSManager::HandleSpeekSC(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
    const auto& rmb = data["rmb"].get<int>();
    const auto& message = data["message"].get<std::string>();
    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 赠送的") + std::to_wstring(rmb) + TEXT("元SC：") + utf8_to_wstring(message);
    HistoryLogMsgQueue.push_back(msg);
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekGuard(const json& data)
{
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
    HistoryLogMsgQueue.push_back(msg);
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekEnter(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
    TString msg = utf8_to_wstring(uname) + TEXT(" 进入直播间");
    HistoryLogMsgQueue.push_back(msg);
}

bool TTSManager::Speak(const TString& text)
{
    LOG_INFO(TEXT("=== TTS Speak called with text: %s ==="), text.c_str());
    
#if USE_MIMO_TTS
    TTSEngine engine = GetActiveEngine();
    
    LOG_INFO(TEXT("TTS Engine: %d, isFallback: %d"), (int)engine, (int)isFallback);
    
    if (engine == TTSEngine::MIMO || (engine == TTSEngine::AUTO && !isFallback)) {
        LOG_INFO(TEXT("Submitting async MiMo TTS for: %s"), text.c_str());
        // 异步提交到MiMo TTS队列，立即返回（不阻塞调用线程）
        SpeakWithMimoAsync(text);
        return true;  // 返回true表示请求已提交（不代表播放完成）
    }
#endif
    
    // 使用Windows SAPI
    LOG_INFO(TEXT("Using SAPI fallback"));
    return SpeakWithSapi(text);
}

bool TTSManager::PlayAudioData(const std::vector<uint8_t>& audioData, const std::string& format) {
#if USE_MIMO_TTS
    if (audioPlayer == NULL) {
        LOG_ERROR(TEXT("PlayAudioData: audioPlayer is NULL"));
        return false;
    }
    bool success = audioPlayer->Play(audioData, format);
    if (!success) {
        LOG_ERROR(TEXT("PlayAudioData: AudioPlayer::Play failed"));
    }
    return success;
#else
    LOG_WARNING(TEXT("PlayAudioData: MIMO TTS not enabled, using SAPI"));
    return false;
#endif
}

void TTSManager::SpeakCheckinTTS(const TString& text, const std::string& username) {
#if USE_MIMO_TTS
    if (!ttsProvider) {
        LOG_ERROR(TEXT("SpeakCheckinTTS: ttsProvider is NULL"));
        return;
    }
    
    if (!ttsProvider->IsAvailable()) {
        LOG_WARNING(TEXT("SpeakCheckinTTS: MiMo TTS not available"));
        return;
    }
    
    AsyncTTSRequest req;
    req.text = text;
    req.state = AsyncTTSState::Pending;
    req.startTime = std::chrono::steady_clock::now();
    req.responseFormat = "mp3";
    req.isCheckinTTS = true;
    req.checkinUsername = username;

    {
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        asyncPendingQueue_.push_back(req);
    }
    LOG_DEBUG(TEXT("SpeakCheckinTTS: Request added to queue for: %s"), text.c_str());
#endif
}

bool TTSManager::SpeakWithSapi(const TString& text)
{
    std::lock_guard<std::mutex> lock(sapiMutex_);

    LOG_DEBUG(TEXT("SpeakWithSapi: pVoice=%p"), pVoice);

    if (pVoice == NULL) {
        LOG_ERROR(TEXT("SpeakWithSapi: pVoice is NULL, cannot speak"));
        return false;
    }

    {
        ISpObjectToken* pChineseToken = NULL;
        HRESULT hrToken = SpFindBestToken(SPCAT_VOICES, L"Language=804", NULL, &pChineseToken);
        if (SUCCEEDED(hrToken) && pChineseToken) {
            pVoice->SetVoice(pChineseToken);
            LOG_DEBUG(TEXT("SpeakWithSapi: Set Chinese voice"));
            pChineseToken->Release();
        } else {
            LOG_DEBUG(TEXT("SpeakWithSapi: Chinese voice not found, hr=0x%08X"), hrToken);
        }
    }

    int speechRate = ConfigManager::Inst()->GetConfig().speechRate;
    int speechVolume = ConfigManager::Inst()->GetConfig().speechVolume;
    int pitch = ConfigManager::Inst()->GetConfig().speechPitch;
    LOG_DEBUG(TEXT("SpeakWithSapi: rate=%d, volume=%d, pitch=%d"), speechRate, speechVolume, pitch);

    pVoice->SetRate(speechRate);
    pVoice->SetVolume(speechVolume);
    std::wstring pitchStr = (pitch >= 0 ? L"+" : L"") + std::to_wstring(pitch) + L"st";

    // Escape '<' and '&' in text to prevent SSML/XML parsing issues
    std::wstring safeText;
    safeText.reserve(text.size());
    for (wchar_t ch : text) {
        if (ch == L'<') {
            safeText += L"&lt;";
        }
        else if (ch == L'&') {
            safeText += L"&amp;";
        }
        else if (ch == L'>') {
            safeText += L"&gt;";
        }
        else {
            safeText += ch;
        }
    }

    std::wstring ssml = L"<speak version='1.0' xml:lang='zh-CN'><prosody pitch='" + pitchStr + L"'>" + safeText + L"</prosody></speak>";
    HRESULT hr = pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL);
    LOG_DEBUG(TEXT("SpeakWithSapi: Speak result=0x%08X"), hr);
    return SUCCEEDED(hr);
}

#if USE_MIMO_TTS
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
    AsyncTTSRequest req;
    req.text = text;
    req.state = AsyncTTSState::Pending;
    req.startTime = std::chrono::steady_clock::now();
    req.callback = callback;

    // 获取配置参数
    const auto& config = ConfigManager::Inst()->GetConfig();
    if (!config.mimoAudioFormat.empty()) {
        req.responseFormat = config.mimoAudioFormat;
    } else {
        req.responseFormat = "mp3";
    }

    // 需要加锁保护asyncPendingQueue_
    size_t queueSize;
    {
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        asyncPendingQueue_.push_back(req);
        queueSize = asyncPendingQueue_.size();
    }
    LOG_DEBUG(TEXT("SpeakWithMimoAsync: Request added to queue, queue size: %zu"), queueSize);
}

void TTSManager::ProcessAsyncTTS()
{
    // 处理所有当前活跃的请求（需要加锁保护asyncPendingQueue_）
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    
    // 处理活跃请求：遍历当前活跃的请求
    int processed = 0;
    while (processed < activeRequestCount_ && !asyncPendingQueue_.empty()) {
        auto it = asyncPendingQueue_.begin();
        AsyncTTSRequest& req = *it;
        switch (req.state) {
        case AsyncTTSState::Pending:
            ProcessPendingRequest(it);
            processed++;
            break;
        case AsyncTTSState::Requesting:
            ProcessRequestingState(req);
            processed++;
            break;
        case AsyncTTSState::Playing:
            ProcessPlayingState(req);
            processed++;
            break;
        case AsyncTTSState::Completed:
        case AsyncTTSState::Failed:
            LOG_INFO(TEXT("TTS Async: Request %s, cleaning up"),
                req.state == AsyncTTSState::Completed ? TEXT("completed") : TEXT("failed"));
            asyncPendingQueue_.pop_front();
            activeRequestCount_--;
            processed++;
            break;
        }
    }

    // 如果未达到并发上限，从队列取新的请求
    while (activeRequestCount_ < MAX_CONCURRENT_TTS && !asyncPendingQueue_.empty()) {
        AsyncTTSRequest& req = asyncPendingQueue_.front();
        if (req.state != AsyncTTSState::Pending) {
            break;
        }
        req.startTime = std::chrono::steady_clock::now();
        activeRequestCount_++;
        LOG_INFO(TEXT("TTS Async: Starting new request for: %s (active: %d)"), 
            req.text.c_str(), activeRequestCount_.load());
        ProcessPendingRequest(asyncPendingQueue_.begin());
    }
}

void TTSManager::ProcessPendingRequest(std::list<AsyncTTSRequest>::iterator it)
{
    AsyncTTSRequest& req = *it;
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
    // 注意：不能捕获req的引用，因为回调执行时req可能已经被pop_front()
    // 解决方案：通过传入的迭代器找到对应的请求
    std::lock_guard<std::recursive_mutex> queueLock(asyncMutex_);
    if (asyncPendingQueue_.empty()) {
        LOG_ERROR(TEXT("TTS Async: Request queue is empty, cannot send request"));
        return;
    }
    ttsProvider->RequestTTS(ttsReq, [this, it](const TTSResponse& response) {
        // 回调在HTTP线程中执行，需要线程安全地修改状态
        std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
        if (it == asyncPendingQueue_.end()) {
            return;  // 请求已被清理
        }
        if (response.success && !response.audioData.empty()) {
            it->audioData = response.audioData;
            it->state = AsyncTTSState::Playing;
            LOG_INFO(TEXT("TTS Async: API request succeeded, starting playback"));
            
            if (it->isCheckinTTS && !it->checkinUsername.empty()) {
                TTSCacheManager::Inst()->SaveCheckinAudio(it->checkinUsername, response.audioData, GetTickCount64());
            }
            if (it->callback) {
                it->callback(true, "");
            }
        } else {
            it->errorMessage = response.errorMsg;
            it->state = AsyncTTSState::Failed;
            LOG_ERROR(TEXT("TTS Async: API request failed: %s"), utf8_to_wstring(response.errorMsg).c_str());
            if (it->callback) {
                it->callback(false, response.errorMsg);
            }
        }
    });

    req.state = AsyncTTSState::Requesting;
    req.startTime = std::chrono::steady_clock::now();
}

void TTSManager::ProcessRequestingState(AsyncTTSRequest& req)
{
    // 使用锁检查状态，避免与回调竞态
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    
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
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - req.startTime).count();

    if (elapsed >= API_TIMEOUT_SECONDS) {
        LOG_WARNING(TEXT("TTS Async: API request timeout (%d seconds)"), elapsed);
        // 不在这里标记失败，因为HTTP回调可能还在处理中
        // 等到下次Tick，如果audioData有数据会转为Playing，没有才会真正失败
        // 为了避免无限等待，最多等待2个额外的超时周期
        if (elapsed >= API_TIMEOUT_SECONDS * 3) {
            HandleRequestFailure(req);
        }
    }
}

void TTSManager::ProcessPlayingState(AsyncTTSRequest& req)
{
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    
    if (audioPlayer == NULL) {
        req.state = AsyncTTSState::Failed;
        req.errorMessage = "audioPlayer is NULL";
        return;
    }

    // 播放音频（非阻塞模式）
    if (!req.audioData.empty() && !audioPlayer->IsPlaying() && !req.playbackStarted) {
        bool playSuccess = audioPlayer->Play(req.audioData, req.responseFormat);
        if (!playSuccess) {
            LOG_ERROR(TEXT("TTS Async: Audio playback failed"));
            req.state = AsyncTTSState::Failed;
            return;
        }
        req.playbackStarted = true;
        req.audioData.clear();
        req.startTime = std::chrono::steady_clock::now();  // 重置超时计时
        LOG_INFO(TEXT("TTS Async: Audio playback started"));
    }

    // 检查播放完成：只有真正开始播放了才检查完成状态
    if (req.playbackStarted) {
        bool playbackComplete = audioPlayer->IsPlaybackComplete();
        
        bool timedOut = false;
        if (PLAYBACK_TIMEOUT_SECONDS > 0) {
            auto now = std::chrono::steady_clock::now();
            auto playbackElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - req.startTime).count();
            timedOut = (playbackElapsed > PLAYBACK_TIMEOUT_SECONDS * 1000);
        }
        
        if (playbackComplete || timedOut) {
            if (playbackComplete) {
                LOG_INFO(TEXT("TTS Async: Playback completed (MCI reported stop)"));
            } else {
                LOG_WARNING(TEXT("TTS Async: Playback timeout (%d ms), treating as completed"), 
                    (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - req.startTime).count());
                if (audioPlayer != NULL) {
                    audioPlayer->Stop();
                }
            }
            if (audioPlayer != NULL) {
                audioPlayer->CleanupTempFile();
            }
            req.state = AsyncTTSState::Completed;
            consecutiveFailures++;
            lastFailureTime = std::chrono::steady_clock::now();
            return;
        }
    }
}

void TTSManager::HandleRequestFailure(AsyncTTSRequest& req)
{
    // 重试逻辑
    if (req.retryCount < MAX_RETRY_COUNT) {
        req.retryCount++;
        req.state = AsyncTTSState::Pending;  // 重置为Pending，重新请求
        LOG_WARNING(TEXT("TTS Async: Retrying request (%d/%d)"), req.retryCount, MAX_RETRY_COUNT);
        return;
    }

    // 重试次数用尽，标记失败
    req.state = AsyncTTSState::Failed;
    LOG_ERROR(TEXT("TTS Async: Request failed after %d retries"), req.retryCount);

    // 记录失败，用于降级判断
    consecutiveFailures++;
    lastFailureTime = std::chrono::steady_clock::now();

    if (consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
        TriggerFallback();
    }

    // 不在这里降级到SAPI，因为可能有其他请求正在播放
    // audioPlayer是单例，调用Stop()会打断其他请求的播放
    // 让失败的请求直接失败，不尝试SAPI降级播放
}

void TTSManager::CleanupCompletedRequests()
{
    // 清理Completed和Failed状态的请求
    std::lock_guard<std::recursive_mutex> lock(asyncMutex_);
    while (!asyncPendingQueue_.empty()) {
        auto& front = asyncPendingQueue_.front();
        if (front.state == AsyncTTSState::Completed || front.state == AsyncTTSState::Failed) {
            asyncPendingQueue_.pop_front();
        } else {
            break;  // 队列是FIFO，前面的已完成才清理
        }
    }
}
#endif

bool TTSManager::IsUsingMimoTTS() const
{
#if USE_MIMO_TTS
    TTSEngine engine = GetActiveEngine();
    return (engine == TTSEngine::MIMO || (engine == TTSEngine::AUTO && !isFallback));
#else
    return false;
#endif
}

void TTSManager::RefreshEngineStatus()
{
#if USE_MIMO_TTS
    isFallback = false;
    consecutiveFailures = 0;
    LOG_INFO(TEXT("TTS engine status refreshed"));
#endif
}

TTSManager::TTSEngine TTSManager::GetActiveEngine() const
{
#if USE_MIMO_TTS
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
#else
    return TTSEngine::SAPI;
#endif
}

void TTSManager::TriggerFallback()
{
#if USE_MIMO_TTS
    if (!isFallback) {
        isFallback = true;
        fallbackReason = "Consecutive failures exceeded limit";
        LOG_WARNING(TEXT("TTS engine fallback triggered: switching to SAPI after %d consecutive failures"),
            consecutiveFailures);
    }
#endif
}

void TTSManager::TryRecovery()
{
#if USE_MIMO_TTS
    if (!ttsProvider) {
        return;
    }

    LOG_INFO(TEXT("Attempting MiMo TTS recovery..."));
    
    if (ttsProvider->IsAvailable()) {
        isFallback = false;
        consecutiveFailures = 0;
        lastRecoveryAttempt = std::chrono::steady_clock::now();
        LOG_INFO(TEXT("MiMo TTS recovery successful"));
    } else {
        lastRecoveryAttempt = std::chrono::steady_clock::now();
        LOG_WARNING(TEXT("MiMo TTS recovery failed - API still not available"));
    }
#endif
}

bool TTSManager::ShouldTryRecovery() const
{
#if USE_MIMO_TTS
    if (!isFallback) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRecoveryAttempt).count();
    return elapsed >= RECOVERY_INTERVAL_SECONDS;
#else
    return false;
#endif
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
    std::wstring msgWithoutPrefix = msg.substr(2);
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 60);
    int randomValue = dist(rng);
    NormalMsgQueue.push_back(uname + TEXT(" 下单的 ") + msgWithoutPrefix + TEXT(" 已接单，预计") + std::to_wstring(randomValue) + TEXT("分钟后送达！"));
}
