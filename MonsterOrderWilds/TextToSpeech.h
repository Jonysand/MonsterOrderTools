#pragma once
#include "framework.h"
#include <mutex>
#include <list>

#if USE_MIMO_TTS
#include "TTSProvider.h"
#include "AudioPlayer.h"
#include "TTSCacheManager.h"
#endif

struct ISpVoice;

// 异步TTS状态枚举
enum class AsyncTTSState
{
	Pending,        // 等待处理
	Requesting,     // 正在请求API
	Playing,        // 正在播放音频
	Completed,      // 完成
	Failed          // 失败
};

// 异步TTS请求结构
struct AsyncTTSRequest
{
	TString text;                           // 播报文本
	AsyncTTSState state = AsyncTTSState::Pending;  // 当前状态
	std::vector<uint8_t> audioData;        // API返回的音频数据
	std::string responseFormat;            // 音频格式
	std::chrono::steady_clock::time_point startTime;  // 状态开始时间
	int retryCount = 0;                     // 重试次数
	std::string errorMessage;               // 错误信息
	bool isCheckinTTS = false;              // 是否为checkin TTS
	std::string checkinUsername;            // checkin用户名（用于缓存）
	std::string voice;                     // 语音
	int speed = 0;                          // 语速
	std::function<void(bool success, const std::string& errorMsg)> callback;  // 完成回调
};

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
	// 使用SAPI进行语音播报
	bool SpeakWithSapi(const TString& text);

public:
	bool Speak(const TString& text);
	bool PlayAudioData(const std::vector<uint8_t>& audioData, const std::string& format = "mp3");
	void SpeakCheckinTTS(const TString& text, const std::string& username);
private:
#if USE_MIMO_TTS
	// 异步TTS方法
	void SpeakWithMimoAsync(const TString& text, std::function<void(bool success, const std::string& errorMsg)> callback = nullptr);
	// 处理异步TTS状态机
	void ProcessAsyncTTS();
	// 处理Pending状态 - 发起API请求
	void ProcessPendingRequest(std::list<AsyncTTSRequest>::iterator it);
	// 处理Requesting状态 - 检查API响应
	void ProcessRequestingState(AsyncTTSRequest& req);
	// 处理Playing状态 - 检查播放完成
	void ProcessPlayingState(AsyncTTSRequest& req);
	// 处理失败/超时
	void HandleRequestFailure(AsyncTTSRequest& req);
	// 清理已完成/失败的请求
	void CleanupCompletedRequests();
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
	std::mutex sapiMutex_;
	std::recursive_mutex asyncMutex_;  // 保护异步TTS请求状态（使用递归锁以便在ProcessAsyncTTS锁内调用子函数）

#if USE_MIMO_TTS
	// TTS提供者
	std::unique_ptr<ITTSProvider> ttsProvider;
	// 音频播放器
	AudioPlayer* audioPlayer{ NULL };

	// 异步TTS请求队列（多并发处理，最多MAX_CONCURRENT_TTS个在处理）
	std::list<AsyncTTSRequest> asyncPendingQueue_;      // 等待队列
	std::atomic<int> activeRequestCount_{ 0 };   // 当前正在处理的请求数量
	static constexpr int MAX_CONCURRENT_TTS = 3;       // 最大并发请求数
	static constexpr int MAX_ASYNC_QUEUE_SIZE = 0;      // 队列大小限制（0=不限制）
	static constexpr int MAX_RETRY_COUNT = 0;           // 最大重试次数
	static constexpr int API_TIMEOUT_SECONDS = 5;       // API请求超时（秒）
	static constexpr int PLAYBACK_TIMEOUT_SECONDS = 3; // 播放超时（秒）

	static constexpr int GIFT_COOLDOWN_SECONDS = 5;
	static constexpr int DYNAMIC_COMBO_WINDOW_SECONDS = 10;

	struct DynamicComboEntry {
		std::string combo_id;
		std::string uname;
		std::string gift_name;
		int gift_num;
		float combo_timeout;
		bool paid;
		bool firstReported;
		int64_t lastUpdateTime;
		
		DynamicComboEntry() : gift_num(0), combo_timeout(0), paid(false), firstReported(false), lastUpdateTime(0) {}
	};
	
	std::unordered_map<std::string, DynamicComboEntry> dynamicComboMap_;
	std::unordered_map<std::string, int64_t> giftCooldownMap_;
	
	bool IsInCooldown(const std::string& comboId);
	void UpdateCooldown(const std::string& comboId);
	void CleanupExpiredCooldowns();
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