#pragma once
#include "framework.h"

#if USE_MIMO_TTS
#include "MimoTTSClient.h"
#include "AudioPlayer.h"
#endif

struct ISpVoice;

class TTSManager
{
	DECLARE_SINGLETON(TTSManager)
	
	struct ComboGiftMsgEntry
	{
		// Api中的连击时间
		// 在这里用来作为放入GiftMsgQueue之前需要在GiftMsgPrepareMap中待的时间，为0时结算加入GiftMsgQueue
		float combo_timeout = 3;
		// 总共送出的礼物数量，combo_base_num * combo_count
		int gift_num;
		// 用户名
		std::string uname;
		// 礼物名
		std::string gift_name;
		// 是否收费
		bool paid;
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

	// 检查当前使用的TTS引擎
	bool IsUsingMimoTTS() const;
	// 强制刷新引擎状态（用于配置变更后）
	void RefreshEngineStatus();

private:
	bool Speak(const TString& text);
	bool SpeakWithSapi(const TString& text);
#if USE_MIMO_TTS
	bool SpeakWithMimo(const TString& text);
#endif

	void HandleDmOrderFood(const std::wstring& text, const std::wstring& uname);

	// 引擎选择和降级逻辑
	enum class TTSEngine {
		SAPI,       // Windows SAPI
		MIMO,       // 小米MiMo
		AUTO        // 自动模式（优先MiMo，失败时降级）
	};
	TTSEngine GetActiveEngine() const;
	void TriggerFallback();
	void TryRecovery();
	bool ShouldTryRecovery() const;

	// 一般弹幕播报队列
	std::list<TString> NormalMsgQueue;
	// 送礼弹幕播报队列
	std::list<TString> GiftMsgQueue;
	// 送礼弹幕播报准备队列，处理连击送礼
	std::map<std::string, ComboGiftMsgEntry> ComboGiftMsgPrepareMap;
	// 历史记录队列
	std::list<TString> HistoryLogMsgQueue;

	// 每次tick更新时间戳
	std::chrono::steady_clock::time_point LastTickTime;
	ISpVoice* pVoice{ NULL };

#if USE_MIMO_TTS
	// 小米MiMo TTS客户端
	MimoTTSClient* mimoClient{ NULL };
	// 音频播放器
	AudioPlayer* audioPlayer{ NULL };
#endif

	// 引擎降级相关
	bool isFallback{ false };              // 是否已降级到SAPI
	std::string fallbackReason;            // 降级原因
	int consecutiveFailures{ 0 };          // 连续失败次数
	std::chrono::steady_clock::time_point lastFailureTime;  // 上次失败时间
	std::chrono::steady_clock::time_point lastRecoveryAttempt;  // 上次恢复尝试时间
	static constexpr int MAX_CONSECUTIVE_FAILURES = 3;  // 最大连续失败次数
	static constexpr int RECOVERY_INTERVAL_SECONDS = 300;  // 恢复尝试间隔（秒）
};