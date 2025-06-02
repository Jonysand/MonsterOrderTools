#pragma once
#include "framework.h"
#include "Network.h"
#include "EventSystem.h"



// Manager Class, Singleton
class BliveManager
{
public:
    static BliveManager* Inst();
    static void Destroy();

    /*
    * 直播连接------------------------------------------------------------------
    */
public:
    // 链接弹幕服务器
    void Start(const std::string& IdCode="");
	// 关闭弹幕服务器
	// GameId: 为空则使用当前场次
	// instantly: 是否立即关闭
	void End(const std::string& GameId = "", bool instantly = false);
	// Tick
    void Tick();
    // IsConnected
    inline bool IsConnected() { return connected.load(); };
    inline void SetConnected(bool val) { connected.store(val); };
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
private:
    std::atomic<bool> connected{ false };
    // 直播身份码
    std::string idCode{ "" };
    // 本场次gameID
	std::string gameId;
	// 直播服务器地址
	std::vector<std::string> serverAddresses;
    // auth体
	std::string authBody;
    // 长链接
    HINTERNET webSocket;
    // 正在进行的http协程
	std::list<Network::NetworkCoroutine> networkCoroutines;
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
            func();
            return true;
        }
    };
    std::list<delayTask> delayedTasks;
    // websocket 信息
    std::queue<ProtoUtils::Packet> wsMessages;
    Lock wsMsgLock;
    

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


    static BliveManager* __Instance;
};
