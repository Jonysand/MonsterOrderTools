#pragma once
#include "framework.h"

struct ISpVoice;

class TTSManager
{
	DECLARE_SINGLETON(TTSManager)
	
	struct ComboGiftMsgEntry
	{
		// Api�е�����ʱ��
		// ������������Ϊ����GiftMsgQueue֮ǰ��Ҫ��GiftMsgPrepareMap�д���ʱ�䣬Ϊ0ʱ�������GiftMsgQueue
		float combo_timeout;
		// �ܹ��ͳ�������������combo_base_num * combo_count
		int gift_num;
		// �û���
		std::string uname;
		// ������
		std::string gift_name;
	};

public:
	TTSManager();
	~TTSManager();

	void Tick();

	// ������Ļ
	void HandleSpeekDm(const json& data);
	// ��������
	void HandleSpeekSendGift(const json& data);
	// ����SC
	void HandleSpeekSC(const json& data);
	// �����Ͻ�
	void HandleSpeekGuard(const json& data);
	// �������뷿��
	void HandleSpeekEnter(const json& data);
private:
	bool Speak(const TString& text);

	// һ�㵯Ļ��������
	std::list<TString> NormalMsgQueue;
	// ����Ļ��������
	std::list<TString> GiftMsgQueue;
	// ����Ļ����׼�����У�������������
	std::map<std::string, ComboGiftMsgEntry> ComboGiftMsgPrepareMap;

	// ÿ��tick����ʱ���
	std::chrono::steady_clock::time_point LastTickTime;
	ISpVoice* pVoice{ NULL };
};