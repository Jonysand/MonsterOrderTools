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

void TTSManager::HandleSpeekDm(const json& data)
{
    const auto& wearing_medal = data["fans_medal_wearing_status"].get<bool>();
    const auto& guard_level = data["guard_level"].get<int>();
    if (GET_CONFIG(ONLY_SPEEK_WEARING_MEDAL) && !wearing_medal)
        return;
    if (GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL) != 0 && (guard_level == 0 || guard_level > GET_CONFIG(ONLY_SPEEK_GUARD_LEVEL)))
        return;
    const auto& uname = data["uname"].get<std::string>();
    const auto& msg = data["msg"].get<std::string>();
    Speak(utf8_to_wstring(uname) + TEXT("说：") + utf8_to_wstring(msg));
}

void TTSManager::HandleSpeekSendGift(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
    const auto& gift_name = data["gift_name"].get<std::string>();
    const auto& gift_num = data["gift_num"].get<int>();
    const auto& paid = data["paid"].get<bool>();
    if (GET_CONFIG(ONLY_SPEEK_PAID_GIFT) && !paid)
        return;
    Speak(TEXT("感谢苍蓝星") + utf8_to_wstring(uname) + TEXT("赠送的") + std::to_wstring(gift_num) + TEXT("个") + utf8_to_wstring(gift_name));
}

void TTSManager::HandleSpeekSC(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
    const auto& rmb = data["rmb"].get<int>();
    const auto& message = data["message"].get<std::string>();
    Speak(TEXT("感谢苍蓝星") + utf8_to_wstring(uname) + TEXT("赠送的") + std::to_wstring(rmb) + TEXT("元SC：") + utf8_to_wstring(message));
}

void TTSManager::HandleSpeekGuard(const json& data)
{
    const auto& uname = data["uname"].get<std::string>();
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
    Speak(TEXT("感谢苍蓝星") + utf8_to_wstring(uname) + TEXT("上船") + std::to_wstring(guard_num) + utf8_to_wstring(guard_unit) + TEXT("的") + guard_name);
}

bool TTSManager::Speak(const TString& text)
{
    pVoice->SetRate(GET_CONFIG(SPEECH_RATE));
    return SUCCEEDED(pVoice->Speak(text.c_str(), SPF_ASYNC, NULL));
}