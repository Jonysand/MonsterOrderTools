#pragma once
#include "framework.h"

struct ISpVoice;

class TTSManager
{
	DECLARE_SINGLETON(TTSManager)
	
public:
	TTSManager();
	~TTSManager();

	// ²¥±¨µ¯Ä»
	void HandleSpeekDm(const json& data);
	// ²¥±¨ËÍÀñ
	void HandleSpeekSendGift(const json& data);
	// ²¥±¨SC
	void HandleSpeekSC(const json& data);
	// ²¥±¨ÉÏ½¢
	void HandleSpeekGuard(const json& data);
private:
	bool Speak(const TString& text);

	ISpVoice* pVoice{ NULL };
};