#include "TextToSpeech.h"
#include "Network.h"
#include "MHDanmuToolsHost.h"
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
    mimoClient = new MimoTTSClient();
    audioPlayer = new AudioPlayer();
#endif
}

TTSManager::~TTSManager()
{
#if USE_MIMO_TTS
    if (mimoClient != NULL) {
        delete mimoClient;
        mimoClient = NULL;
    }
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
#endif

    if (!NormalMsgQueue.empty())
    {
        if (GET_CONFIG(ENABLE_VOICE))
            Speak(NormalMsgQueue.front());
        NormalMsgQueue.pop_front();
    }
    if (!GiftMsgQueue.empty())
    {
        if (GET_CONFIG(ENABLE_VOICE))
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
            if (GET_CONFIG(ENABLE_VOICE) && (!GET_CONFIG(ONLY_SPEEK_PAID_GIFT) || it->second.paid))
                GiftMsgQueue.push_back(msg);
            HistoryLogMsgQueue.push_back(msg);
            it = ComboGiftMsgPrepareMap.erase(it);
        }
        else
            ++it;
    }
}

void TTSManager::HandleSpeekDm(const json& data)
{
    const auto& wearing_medal = data["fans_medal_wearing_status"].get<bool>();
    const auto& guard_level = data["guard_level"].get<int>();
    const auto& uname = data["uname"].get<std::string>();
    const auto& msg = utf8_to_wstring(data["msg"].get<std::string>());
    TString msgTString = utf8_to_wstring(uname) + TEXT(" 说：") + msg;
    HistoryLogMsgQueue.push_back(msgTString);
    if (GET_CONFIG(ONLY_SPEEK_WEARING_MEDAL) && !wearing_medal)
        return;
    if (GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL) != 0 && (guard_level == 0 || guard_level > GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL)))
        return;
    if (msg.rfind(TEXT("点餐"), 0) == 0) {
        // 以"点餐"开头
        HandleDmOrderFood(msg, utf8_to_wstring(uname));
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

    std::string combo_id;
    int combo_timeout = 3;
    if (paid)
    {
        combo_id = data["combo_info"]["combo_id"].get<std::string>();
        combo_timeout = data["combo_info"]["combo_timeout"].get<int>();
        gift_num = data["combo_info"]["combo_base_num"].get<int>() * data["combo_info"]["combo_count"].get<int>();
    }
    else
    {
        const auto& open_id = data["open_id"].get<std::string>();
        std::string gift_id = std::to_string(data["gift_id"].get<int>());
        combo_id = open_id + gift_id;
    }
    auto it = ComboGiftMsgPrepareMap.find(combo_id);
    if (it != ComboGiftMsgPrepareMap.end())
    {
        it->second.combo_timeout = combo_timeout;
        if (paid)
            it->second.gift_num = gift_num;
        else
            it->second.gift_num += gift_num;
    }
    else
        ComboGiftMsgPrepareMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(std::move(combo_id)),
            std::forward_as_tuple(combo_timeout, gift_num, uname, gift_name, paid)
        );
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
#if USE_MIMO_TTS
    TTSEngine engine = GetActiveEngine();
    
    LOG_INFO(TEXT("TTS Engine: %d, isFallback: %d"), (int)engine, (int)isFallback);
    
    if (engine == TTSEngine::MIMO || (engine == TTSEngine::AUTO && !isFallback)) {
        LOG_INFO(TEXT("Trying MiMo TTS for: %s"), text.c_str());
        // 尝试使用小米MiMo TTS
        if (SpeakWithMimo(text)) {
            consecutiveFailures = 0;  // 重置失败计数
            return true;
        } else {
            LOG_WARNING(TEXT("MiMo TTS failed, falling back to SAPI"));
            // MiMo失败
            consecutiveFailures++;
            lastFailureTime = std::chrono::steady_clock::now();
            
            if (engine == TTSEngine::AUTO && consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
                // 触发降级
                TriggerFallback();
            }
            
            // 尝试使用SAPI作为备选
            return SpeakWithSapi(text);
        }
    }
#endif
    
    // 使用Windows SAPI
    return SpeakWithSapi(text);
}

bool TTSManager::SpeakWithSapi(const TString& text)
{
    if (pVoice == NULL) {
        return false;
    }

    pVoice->SetRate(GET_CONFIG(SPEECH_RATE));
    pVoice->SetVolume(GET_CONFIG(SPEECH_VOLUME));
    int pitch = GET_CONFIG(SPEECH_PITCH);
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
    return SUCCEEDED(pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL));
}

#if USE_MIMO_TTS
bool TTSManager::SpeakWithMimo(const TString& text)
{
    if (mimoClient == NULL || audioPlayer == NULL) {
        return false;
    }

    if (!mimoClient->IsAvailable()) {
        LOG_WARNING(TEXT("MiMo TTS not available (API Key not configured)"));
        return false;
    }

    // 构建请求参数
    MimoTTSClient::TTSRequest request;
    
    // 获取配置参数
    System::String^ voice = GET_CONFIG(MIMO_VOICE);
    System::String^ style = GET_CONFIG(MIMO_STYLE);
    System::String^ dialect = GET_CONFIG(MIMO_DIALECT);
    System::String^ role = GET_CONFIG(MIMO_ROLE);
    System::String^ format = GET_CONFIG(MIMO_AUDIO_FORMAT);
    float speed = GET_CONFIG(MIMO_SPEED);

    if (voice != nullptr && voice->Length > 0) {
        request.voice = wstring_to_utf8(msclr::interop::marshal_as<std::wstring>(voice));
    }
    
    // 构建文本：将默认风格标签放在"xxx说："前面
    std::string styleTag;
    if (style != nullptr && style->Length > 0) {
        styleTag = "<style>" + wstring_to_utf8(msclr::interop::marshal_as<std::wstring>(style)) + "</style>";
    }
    request.text = styleTag + wstring_to_utf8(text);
    
    // Debug log
    LOG_INFO(TEXT("MiMo TTS Request Text: %s"), text.c_str());
    LOG_INFO(TEXT("MiMo TTS Request Text UTF8: %s"), utf8_to_wstring(request.text).c_str());
    if (dialect != nullptr && dialect->Length > 0) {
        request.dialect = wstring_to_utf8(msclr::interop::marshal_as<std::wstring>(dialect));
    }
    if (role != nullptr && role->Length > 0) {
        request.role = wstring_to_utf8(msclr::interop::marshal_as<std::wstring>(role));
    }
    if (format != nullptr && format->Length > 0) {
        request.responseFormat = wstring_to_utf8(msclr::interop::marshal_as<std::wstring>(format));
    }
    request.speed = speed;

    // 发送请求（同步等待结果）
    bool requestCompleted = false;
    bool requestSuccess = false;

    mimoClient->RequestTTS(request, [&](const MimoTTSClient::TTSResponse& response) {
        if (response.success && !response.audioData.empty()) {
            // 播放音频
            requestSuccess = audioPlayer->Play(response.audioData, request.responseFormat);
            if (requestSuccess) {
                // 等待播放完成（带超时）
                audioPlayer->WaitForCompletion(10000);  // 10秒超时
            }
        } else {
            LOG_ERROR(TEXT("MiMo TTS request failed: %s"), utf8_to_wstring(response.errorMessage).c_str());
            requestSuccess = false;
        }
        requestCompleted = true;
    });

    // 简单的同步等待（实际应该使用异步机制）
    // 注意：这会阻塞调用线程，生产环境应该改进为异步
    int waitCount = 0;
    while (!requestCompleted && waitCount < 100) {
        Sleep(50);
        waitCount++;
    }

    return requestSuccess;
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
    System::String^ engineConfig = GET_CONFIG(TTS_ENGINE);
    if (engineConfig == nullptr) {
        return TTSEngine::AUTO;
    }
    
    std::wstring engineStr = msclr::interop::marshal_as<std::wstring>(engineConfig);
    if (engineStr == L"mimo") {
        return TTSEngine::MIMO;
    } else if (engineStr == L"sapi") {
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
    if (mimoClient == NULL) {
        return;
    }

    LOG_INFO(TEXT("Attempting MiMo TTS recovery..."));
    
    if (mimoClient->IsAvailable()) {
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

void TTSManager::HandleDmOrderFood(const std::wstring& msg, const std::wstring& uname)
{
    std::wstring msgWithoutPrefix = msg.substr(2);
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 60);
    int randomValue = dist(rng);
    NormalMsgQueue.push_back(uname + TEXT(" 下单的 ") + msgWithoutPrefix + TEXT(" 已接单，预计") + std::to_wstring(randomValue) + TEXT("分钟后送达！"));
}
