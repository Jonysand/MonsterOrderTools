#pragma once
#include "framework.h"
#include "Network.h"
#include "EventSystem.h"
#include "WriteLog.h"
#include <mutex>
#include <condition_variable>


enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    ReconnectFailed
};

enum class DisconnectReason {
    None,
    NetworkError,
    HeartbeatTimeout,
    ServerClose,
    AuthFailed
};

inline const char* ConnectionStateToString(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::Reconnecting: return "Reconnecting";
        case ConnectionState::ReconnectFailed: return "ReconnectFailed";
        default: return "Unknown";
    }
}

inline const char* DisconnectReasonToString(DisconnectReason reason) {
    switch (reason) {
        case DisconnectReason::None: return "无";
        case DisconnectReason::NetworkError: return "网络错误";
        case DisconnectReason::HeartbeatTimeout: return "心跳超时";
        case DisconnectReason::ServerClose: return "服务器断开";
        case DisconnectReason::AuthFailed: return "鉴权失败";
        default: return "未知";
    }
}


static constexpr int MAX_RECONNECT_ATTEMPTS = 99999;
static constexpr uint64_t RECONNECT_BASE_DELAY_MS = 1000;
static constexpr uint64_t RECONNECT_MAX_DELAY_MS = 60000;
static constexpr uint64_t WS_HEARTBEAT_RECONNECT_DELAY_MS = 2000;
static constexpr uint64_t INITIAL_HEARTBEAT_DELAY_MS = 5000;


class BliveManager
{
    DECLARE_SINGLETON(BliveManager)

    /*
    * 直播连接------------------------------------------------------------------
    */
public:
    // 链接弹幕服务器
    void Start(const std::string& IdCode="");
	// 关闭弹幕服务器
	// GameId: 为空则使用当前场次
	// restart: 结束后是否自动重连
	void End(const std::string& GameId = "", bool restart = false);
	// Tick
    void Tick();
    // IsConnected
    inline bool IsConnected() { 
        return !GetDestroyingFlag().load() && connectionState.load() == ConnectionState::Connected; 
    };
    // Get connection state
    inline ConnectionState GetConnectionState() { return connectionState.load(); };
    inline DisconnectReason GetDisconnectReason() { return disconnectReason.load(); };
    inline int GetReconnectAttemptCount() { return reconnectAttemptCount.load(); };
    // Disconnect / Reconnect
    void Disconnect();
    void Reconnect();
    // Set connection state
    void SetConnectionState(ConnectionState state, DisconnectReason reason = DisconnectReason::None);
private:
    BliveManager() = default;
    ~BliveManager();

    // start请求回调
	void OnReceiveStartResponse(const std::string& response);
    // app开始心跳
    void StartAppHeartBeat();
    // app心跳回调
    void OnReceiveAppHeartbeatResponse(const std::string& response);
    // websocket开始心跳
    void StartWebsocketHeartBeat();
    // websocket 接收消息回调，这个函数在子线程
    void OnReceiveWSMessage(HINTERNET hWebsocket, ProtoUtils::Packet message);
    // 处理 websocket 消息
    void HandleWSMessage();
    // 处理一般的弹幕、送礼等消息
    void HandleSmsReply(const std::string& msg);
    // 调度重连（防止重复调度）
    void ScheduleReconnect(uint64_t delayMs);
    // 计算指数退避延迟
    uint64_t CalculateReconnectDelay() const;
private:
    // Async request wrapper for callback-based async operations
    struct AsyncRequest {
        enum Type { HTTP };
        Type type;
        Network::HttpsAsyncCallback httpCallback;
        std::atomic<bool> completed{false};
        std::string response;
        DWORD error = 0;

        void Complete(bool success_, const std::string& response_, DWORD error_) {
            // 数据写入在 CAS 之前，确保对读取端可见
            // 即使 CAS 失败，写入也无害（只有一个线程能通过 CAS）
            if (success_) { response = response_; }
            error = error_;
            bool expected = false;
            if (!completed.compare_exchange_strong(expected, true)) return;
        }
    };

    // 直播身份码
    std::string idCode{ "" };
    // 本场次gameID（需要锁保护）
	std::string gameId;
	mutable std::mutex gameIdLock;
	// 直播服务器地址
	std::vector<std::string> serverAddresses;
    // auth体
	std::string authBody;
    // 长链接
    HINTERNET webSocket = nullptr;
    // 正在进行的异步请求
	std::list<std::shared_ptr<AsyncRequest>> networkRequests;
    // 保护 networkRequests 的锁
    Lock networkRequestsLock;
    // timer管理，延时触发
    struct delayTask
    {
        std::function<void()> func;
        uint64_t timeRemain;
        uint64_t timeStamp;
        delayTask(std::function<void()> Func, uint64_t delay) {
            func = Func;
            timeRemain = delay;
            timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        bool checkInvoke()
        {
            if (timeRemain > 0) {
                uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                uint64_t elapsed = currentTimestamp - timeStamp;
                if (elapsed >= timeRemain) {
                    timeRemain = 0;
                }
                else {
                    timeRemain -= elapsed;
                }
                timeStamp = currentTimestamp;
                return false;
            }
            try {
                func();
            } catch (const std::exception& e) {
                std::string errMsg = e.what();
                LOG_ERROR(TEXT("[BliveManager] delayedTasks callback exception: %s"), ProtoUtils::Decode(errMsg).c_str());
            } catch (...) {
                LOG_ERROR(TEXT("[BliveManager] delayedTasks callback unknown exception"));
            }
            return true;
        }
    };
    std::list<delayTask> delayedTasks;
    // 保护 delayedTasks 的锁
    Lock delayedTasksLock;
    // websocket 信息
    std::queue<ProtoUtils::Packet> wsMessages;
    Lock wsMsgLock;
    // Connection state
    std::atomic<ConnectionState> connectionState{ ConnectionState::Disconnected };
    std::atomic<DisconnectReason> disconnectReason{ DisconnectReason::None };
    std::atomic<int> reconnectAttemptCount{ 0 };
    std::atomic<bool> reconnectScheduled_{ false };
    std::atomic<bool> destroying_{ false };

    /*
    * 事件监听 ----------------------------------------------------------------------------------------
    */
#define REGISTER_EVENT_LISTENER(NAME)   \
public: \
inline void AddListener_##NAME##(const Event<>::Handler& handler) { ##NAME##.AddListener(handler); } \
private: \
    Event<> ##NAME##;

    // 弹幕服务器连接成功
    REGISTER_EVENT_LISTENER(OnBliveConnected)
    // 弹幕服务器断连
    REGISTER_EVENT_LISTENER(OnBliveDisconnected)

#define REGISTER_EVENT_LISTENER_WITH_PARAMS(NAME, P1, P2)   \
public: \
inline void AddListener_##NAME##(const Event<P1, P2>::Handler& handler) { ##NAME##.AddListener(handler); } \
private: \
    Event<P1, P2> ##NAME##;

    // 连接状态变化（带原因）
    REGISTER_EVENT_LISTENER_WITH_PARAMS(OnConnectionStateChanged, ConnectionState, DisconnectReason)

};
