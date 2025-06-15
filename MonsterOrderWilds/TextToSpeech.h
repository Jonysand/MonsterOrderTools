#pragma once
#include "framework.h"

struct ISpVoice;

class TTSManager
{
	DECLARE_SINGLETON(TTSManager)
	
public:
	TTSManager();
	~TTSManager();

	// ������Ļ
	void HandleSpeekDm(const json& data);
	// ��������
	void HandleSpeekSendGift(const json& data);
	// ����SC
	void HandleSpeekSC(const json& data);
	// �����Ͻ�
	void HandleSpeekGuard(const json& data);
private:
	bool Speak(const TString& text);

	ISpVoice* pVoice{ NULL };
};