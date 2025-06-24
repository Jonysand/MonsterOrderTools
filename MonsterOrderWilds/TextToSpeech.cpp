#include "TextToSpeech.h"
#include "Network.h"
#include "MHDanmuToolsHost.h"
#include "WriteLog.h"

#pragma warning(disable: 4996)
#include <sapi.h> // Include SAPI header for ISpVoice
#include <cstringt.h>
#include <sphelper.h>

DEFINE_SINGLETON(TTSManager)

inline std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

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
}

TTSManager::~TTSManager()
{
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

    if (!GET_CONFIG(ENABLE_VOICE))
    {
        NormalMsgQueue.clear();
        GiftMsgQueue.clear();
        ComboGiftMsgPrepareMap.clear();
        return;
    }
    if (!NormalMsgQueue.empty())
    {
        Speak(NormalMsgQueue.front());
        NormalMsgQueue.pop_front();
    }
    if (!GiftMsgQueue.empty())
    {
        Speak(GiftMsgQueue.front());
        GiftMsgQueue.pop_front();
    }
    
    for (auto it = ComboGiftMsgPrepareMap.begin(); it != ComboGiftMsgPrepareMap.end(); )
    {
        it->second.combo_timeout -= deltaTime;
        if (it->second.combo_timeout <= 0.0f)
        {
            TString msg = TEXT("感谢 ") + utf8_to_wstring(it->second.uname) + TEXT(" 赠送的") + std::to_wstring(it->second.gift_num) + TEXT("个") + utf8_to_wstring(it->second.gift_name);
            RECORD_HISTORY(msg.c_str());
            GiftMsgQueue.push_back(msg);
            it = ComboGiftMsgPrepareMap.erase(it);
        }
        else
            ++it;
    }
}

void TTSManager::HandleSpeekDm(const json& data)
{
    if (!GET_CONFIG(ENABLE_VOICE)) return;
    const auto& wearing_medal = data["fans_medal_wearing_status"].get<bool>();
    const auto& guard_level = data["guard_level"].get<int>();
    if (GET_CONFIG(ONLY_SPEEK_WEARING_MEDAL) && !wearing_medal)
        return;
    if (GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL) != 0 && (guard_level == 0 || guard_level > GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL)))
        return;
    const auto& uname = data["uname"].get<std::string>();
    const auto& msg = utf8_to_wstring(data["msg"].get<std::string>());
    TString msgTString = utf8_to_wstring(uname) + TEXT(" 说：") + msg;
    if (msg.rfind(TEXT("点餐"), 0) == 0) {
        // 以"点餐"开头
        std::wstring msgWithoutPrefix = msg.substr(2);
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 60);
        int randomValue = dist(rng);
        msgTString = utf8_to_wstring(uname) + TEXT(" 下单的 ") + msgWithoutPrefix + TEXT(" 已接单，预计") + std::to_wstring(randomValue) + TEXT("分钟后送达！");
    }
    RECORD_HISTORY(msgTString.c_str());
    NormalMsgQueue.push_back(msgTString);
}

void TTSManager::HandleSpeekSendGift(const json& data)
{
    if (!GET_CONFIG(ENABLE_VOICE)) return;
    const auto& paid = data["paid"].get<bool>();
    if (GET_CONFIG(ONLY_SPEEK_PAID_GIFT) && !paid)
        return;
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
            std::forward_as_tuple(combo_timeout, gift_num, uname, gift_name)
        );
}

void TTSManager::HandleSpeekSC(const json& data)
{
    if (!GET_CONFIG(ENABLE_VOICE)) return;
    const auto& uname = data["uname"].get<std::string>();
    const auto& rmb = data["rmb"].get<int>();
    const auto& message = data["message"].get<std::string>();
    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 赠送的") + std::to_wstring(rmb) + TEXT("元SC：") + utf8_to_wstring(message);
    RECORD_HISTORY(msg.c_str());
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekGuard(const json& data)
{
    if (!GET_CONFIG(ENABLE_VOICE)) return;
    const auto& uname = data["user_info"]["uname"].get<std::string>();
    const auto& guard_level = data["guard_level"].get<int>();
    const auto& guard_num = data["guard_num"].get<int>();
    const auto& guard_unit = data["guard_unit"].get<std::string>();
    
    TString guard_name;
    switch (guard_level)
    {
    case 1:
        guard_name = TEXT("总督");
    case 2:
        guard_name = TEXT("提督");
    case 3:
        guard_name = TEXT("舰长");
    default:
        LOG_ERROR(TEXT("Unknown guard level: %d"), guard_level);
        return;
    }
    TString msg = TEXT("感谢 ") + utf8_to_wstring(uname) + TEXT(" 上船") + std::to_wstring(guard_num) + utf8_to_wstring(guard_unit) + TEXT("的") + guard_name;
    RECORD_HISTORY(msg.c_str());
    GiftMsgQueue.push_back(msg);
}

void TTSManager::HandleSpeekEnter(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
    TString msg = utf8_to_wstring(uname) + TEXT(" 进入直播间");
    RECORD_HISTORY(msg.c_str());
}

bool TTSManager::Speak(const TString& text)
{
    pVoice->SetRate(GET_CONFIG(SPEECH_RATE));
    pVoice->SetVolume(GET_CONFIG(SPEECH_VOLUME));
    int pitch = GET_CONFIG(SPEECH_PITCH);
    std::wstring pitchStr = (pitch >= 0 ? L"+" : L"") + std::to_wstring(pitch) + L"st";
    std::wstring ssml = L"<speak version='1.0' xml:lang='zh-CN'><prosody pitch='" + pitchStr + L"'>" + text + L"</prosody></speak>";
    return SUCCEEDED(pVoice->Speak(ssml.c_str(), SPF_IS_XML | SPF_ASYNC, NULL));
}
