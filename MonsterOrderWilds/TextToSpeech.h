#pragma once
#include "framework.h"

struct ISpVoice;

class TTSManager
{
	DECLARE_SINGLETON(TTSManager)
	
	struct ComboGiftMsgEntry
	{
		// Api中的连击时间
		// 在这里用来作为放入GiftMsgQueue之前需要在GiftMsgPrepareMap中待的时间，为0时结算加入GiftMsgQueue
		float combo_timeout;
		// 总共送出的礼物数量，combo_base_num * combo_count
		int gift_num;
		// 用户名
		std::string uname;
		// 礼物名
		std::string gift_name;
	};

public:
	TTSManager();
	~TTSManager();

	void Tick();

	// 播报弹幕
	void HandleSpeekDm(const json& data);
	// 播报送礼
	void HandleSpeekSendGift(const json& data);
	// 播报SC
	void HandleSpeekSC(const json& data);
	// 播报上舰
	void HandleSpeekGuard(const json& data);
	// 播报进入房间
	void HandleSpeekEnter(const json& data);
private:
	bool Speak(const TString& text);

	// 一般弹幕播报队列
	std::list<TString> NormalMsgQueue;
	// 送礼弹幕播报队列
	std::list<TString> GiftMsgQueue;
	// 送礼弹幕播报准备队列，处理连击送礼
	std::map<std::string, ComboGiftMsgEntry> ComboGiftMsgPrepareMap;

	// 每次tick更新时间戳
	std::chrono::steady_clock::time_point LastTickTime;
	ISpVoice* pVoice{ NULL };
};