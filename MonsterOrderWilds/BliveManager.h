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
    * ֱ������------------------------------------------------------------------
    */
public:
    // ���ӵ�Ļ������
    void Start(const std::string& IdCode="");
	// �رյ�Ļ������
	// GameId: Ϊ����ʹ�õ�ǰ����
	// instantly: �Ƿ������ر�
	void End(const std::string& GameId = "", bool instantly = false);
	// Tick
    void Tick();
    // IsConnected
    inline bool IsConnected() { return connected.load(); };
    inline void SetConnected(bool val) { connected.store(val); };
private:
    BliveManager() = default;
    ~BliveManager();

    // start����ص�
	void OnReceiveStartResponse(const std::string& response);
    // app��ʼ����
    void StartAppHeartBeat();
    // app�����ص�
    void OnReceiveAppHeartbeatResponse(const std::string& response);
    // websocket��ʼ����
    void StartWebsocketHeartBeat();
    // websocket ������Ϣ�ص���������������߳�
    void OnReceiveWSMessage(HINTERNET hWebsocket, ProtoUtils::Packet message);
    // ���� websocket ��Ϣ
    void HandleWSMessage();
private:
    std::atomic<bool> connected{ false };
    // ֱ�������
    std::string idCode{ "" };
    // ������gameID
	std::string gameId;
	// ֱ����������ַ
	std::vector<std::string> serverAddresses;
    // auth��
	std::string authBody;
    // ������
    HINTERNET webSocket;
    // ���ڽ��е�httpЭ��
	std::list<Network::NetworkCoroutine> networkCoroutines;
    // timer������ʱ����
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
    // websocket ��Ϣ
    std::queue<ProtoUtils::Packet> wsMessages;
    Lock wsMsgLock;
    

    /*
    * �¼����� ----------------------------------------------------------------------------------------
    */
#define REGISTER_EVENT_LISTENER(NAME)   \
public: \
inline void AddListener_##NAME##(const Event<>::Handler& handler) { ##NAME##.AddListener(handler); } \
private: \
    Event<> ##NAME##;

    // ��Ļ���������ӳɹ�
    REGISTER_EVENT_LISTENER(OnBliveConnected)
    // ��Ļ����������
    REGISTER_EVENT_LISTENER(OnBliveDisconnected)


    static BliveManager* __Instance;
};
