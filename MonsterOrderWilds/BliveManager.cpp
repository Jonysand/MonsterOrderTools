#include "BliveManager.h"
#include "CredentialsConsts.h"
#include "WriteLog.h"
#include "TextToSpeech.h"
#include "MHDanmuToolsHost.h"

DEFINE_SINGLETON(BliveManager)

/*
* ֱ������------------------------------------------------------------------
*/

enum WEBSOCKET_OP {
    OP_UNKNOWN = 0,
    OP_HEARTBEAT = 2,           // �ͻ��˷��͵�������(30�뷢��һ��)
    OP_HEARTBEAT_REPLY = 3,	    // �������յ��������Ļظ�
    OP_SEND_SMS_REPLY = 5,      // ���������͵ĵ�Ļ��Ϣ��
    OP_AUTH = 7,               // �ͻ��˷��͵ļ�Ȩ��(�ͻ��˷��͵ĵ�һ����)
    OP_AUTH_REPLY = 8           // �������յ���Ȩ����Ļظ�
};
void BliveManager::Start(const std::string& IdCode)
{
    if (!IdCode.empty())
        idCode = IdCode;
    if (idCode.empty())
        return;
    std::string params = "{\"code\":\"" + idCode + "\",\"app_id\":" + credentials::APP_ID + "}";
    std::string signedHeader = ProtoUtils::Sign(params);
    networkCoroutines.push_front(std::move(Network::MakeHttpsRequest(
        BLIVE_URL,
        0,
        BLIVE_START_API,
        TEXT("POST"),
        signedHeader,
        params,
        true,
        [this](const std::string& response) { OnReceiveStartResponse(response); }
    )));
}

void BliveManager::End(const std::string& GameId, bool instantly)
{
    std::string params = "{\"game_id\":\"" + (GameId.empty() ? gameId : GameId) + "\",\"app_id\":" + credentials::APP_ID + "}";
    std::string signedHeader = ProtoUtils::Sign(params);
    Network::NetworkCoroutine request = Network::MakeHttpsRequest(
        BLIVE_URL,
        0,
        BLIVE_END_API,
        TEXT("POST"),
        signedHeader,
        params,
        true,
        [](const std::string& response) {
            LOG_DEBUG(TEXT("Stop response: %s"), ProtoUtils::Decode(response).c_str());
        });
    if (instantly)
    {
        while (request.resume())
            continue;
    }
    else
        networkCoroutines.push_front(std::move(request));
}

void BliveManager::Tick() {
    // ����Э��
    if (!networkCoroutines.empty()) {
        auto it = networkCoroutines.begin();
        while (it != networkCoroutines.end()) {
            if (!it->resume())
                it = networkCoroutines.erase(it);
            else
                ++it;
        }
    }
    // ��ʱ����
    if (!delayedTasks.empty()) {
        auto it = delayedTasks.begin();
        while (it != delayedTasks.end()) {
            if (it->checkInvoke())
                it = delayedTasks.erase(it);
            else
                ++it;
        }
    }
    // websocket ��Ϣ
    HandleWSMessage();
}

BliveManager::~BliveManager()
{
    if (!gameId.empty())
        End("", true);
}

void BliveManager::OnReceiveStartResponse(const std::string& response)
{
    LOG_INFO(TEXT("OnStartResponse: %s"), ProtoUtils::Decode(response).c_str());
    DWORD code = -1;
    json jsonResponse;
	try {
		jsonResponse = json::parse(response);
        if (jsonResponse.contains("code"))
            code = jsonResponse["code"];
    }
	catch (const json::parse_error& e) {
		LOG_ERROR(TEXT("JSON parse error: %s"), ProtoUtils::Decode(e.what()).c_str());
        return;
	}
    switch (code)
    {
    case 0:
    {
        TString decodedGameID = ProtoUtils::Decode(jsonResponse["data"]["game_info"]["game_id"].get<std::string>());
        LOG_INFO(TEXT("Game ID: %s"), decodedGameID.c_str());
        // �¼��㲥
        OnBliveConnected.Invoke();
        connected.store(true);
        gameId = ProtoUtils::ConvertTCharToString(decodedGameID.c_str());
        // ׼������websocket
        serverAddresses.clear();
        for (const auto& server : jsonResponse["data"]["websocket_info"]["wss_link"]) {
            std::string serverAddress = server.get<std::string>();
            serverAddresses.push_back(serverAddress);
            LOG_DEBUG(TEXT("Server Address: %s"), ProtoUtils::Decode(serverAddress).c_str());
        }
        authBody = jsonResponse["data"]["websocket_info"]["auth_body"].get<std::string>();
        ProtoUtils::Packet packet;
        packet.op = OP_AUTH;
        packet.body = authBody;
        std::vector<uint8_t> packedData = packet.pack();
        networkCoroutines.push_front(std::move(Network::MakeWebSocketConnection(
            serverAddresses,
            packedData,
            [this](HINTERNET hWebsocket, ProtoUtils::Packet packet) { OnReceiveWSMessage(hWebsocket, std::move(packet)); }
        )));
        // ��ʼapp����
        delayedTasks.push_back({ [this]() { StartAppHeartBeat(); }, 3000 });
        break;
    }
    case 7001:
        LOG_ERROR(TEXT("OnReceiveStartResponse: %s"), TEXT("������ȴ��"));
        break;
    default:
        LOG_ERROR(TEXT("Error OnStartResponse: %s"), ProtoUtils::Decode(response).c_str());
        break;
    }
}

void BliveManager::StartAppHeartBeat()
{
    std::string heartbeatParams = "{\"game_id\":\"" + gameId + "\"}";
    std::string signedHeartbeatHeader = ProtoUtils::Sign(heartbeatParams);
    auto heartBeatCoroutine = Network::MakeHttpsRequest(
        BLIVE_URL,
        0,
        BLIVE_HEARTBEAT_API,
        TEXT("POST"),
        signedHeartbeatHeader,
        heartbeatParams,
        true,
        [this](const std::string& response) { OnReceiveAppHeartbeatResponse(response); }
    );
    networkCoroutines.push_front(std::move(heartBeatCoroutine));
}

void BliveManager::OnReceiveAppHeartbeatResponse(const std::string& response)
{
    LOG_DEBUG(TEXT("OnReceiveAppHeartbeatResponse: %s"), ProtoUtils::Decode(response).c_str());
    DWORD code = -1;
    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
        if (jsonResponse.contains("code"))
            code = jsonResponse["code"];
    }
    catch (const json::parse_error& e) {
        LOG_ERROR(TEXT("JSON parse error: %s"), ProtoUtils::Decode(e.what()).c_str());
        return;
    }
    switch (code)
    {
    case 0:
    {
        // everything is ok
        delayedTasks.push_back({ [this]() { StartAppHeartBeat(); }, HEARBEAT_INTERVAL_MINISECONDS });
        break;
    }
    default:
        OnBliveDisconnected.Invoke();
        connected.store(false);
        LOG_ERROR(TEXT("Error OnReceiveAppHeartbeatResponse: %s"), ProtoUtils::Decode(response).c_str());
        break;
    }
}

void BliveManager::StartWebsocketHeartBeat()
{
    ProtoUtils::Packet packet;
    packet.op = OP_HEARTBEAT;
    std::vector<uint8_t> packedData = packet.pack();
    auto heartBeatCoroutine = Network::SendToWebsocket(
        webSocket,
        packedData
    );
    networkCoroutines.push_front(std::move(heartBeatCoroutine));
}

void BliveManager::OnReceiveWSMessage(HINTERNET hWebsocket, ProtoUtils::Packet packet)
{
    if (!connected.load())
        return;
    // ˵�а汾��ѹ��body�����ﴦ���ѹ��
    wsMsgLock.lock();
    webSocket = hWebsocket;
    wsMessages.push(packet);
    wsMsgLock.unlock();
}

void BliveManager::HandleWSMessage()
{
    uint8_t count = 0;
    wsMsgLock.lock();
    // ����ÿ֡��ദ��10����Ϣ
    while (count < 10 && !wsMessages.empty())
    {
        ProtoUtils::Packet& packet = wsMessages.front();
        WEBSOCKET_OP op = (WEBSOCKET_OP)packet.op;
        bool heartBeatSuccess = true;
        switch (op)
        {
        case OP_HEARTBEAT:
            LOG_ERROR(TEXT("Receive OP_HEARTBEAT from server websocket"));
            break;
        case OP_HEARTBEAT_REPLY:
            if (heartBeatSuccess)
                delayedTasks.push_back({ [this]() { StartWebsocketHeartBeat(); }, HEARBEAT_INTERVAL_MINISECONDS });
            else
            {
                OnBliveDisconnected.Invoke();
                connected.store(false);
            }
            break;
        case OP_SEND_SMS_REPLY:
            HandleSmsReply(packet.body);
            break;
        case OP_AUTH:
            LOG_ERROR(TEXT("Receive OP_AUTH from server websocket"));
            break;
        case OP_AUTH_REPLY:
            LOG_DEBUG(TEXT("OP_AUTH_REPLY"));
            delayedTasks.push_back({ [this]() { StartWebsocketHeartBeat(); }, 2000 });
            break;
        default:
            OnBliveConnected.Invoke();
            connected.store(false);
            LOG_ERROR(TEXT("Receive UNKNOWN from server websocket: %s"), ProtoUtils::Decode(packet.body).c_str());
            break;
        }
        ++count;
        wsMessages.pop();
    }
    wsMsgLock.unlock();
}

void BliveManager::HandleSmsReply(const std::string& msg)
{
    TString decoded = ProtoUtils::Decode(msg);
    LOG_INFO(TEXT("OP_SEND_SMS_REPLY: %s"), decoded.c_str());

    json jsonResponse;
    try {
        jsonResponse = json::parse(msg);
    }
    catch (const json::parse_error& e) {
        LOG_ERROR(TEXT("[HandleSmsReply] JSON parse error: %s"), msg.c_str());
        return;
    }
    const std::string& cmd = jsonResponse["cmd"].get<std::string>();
    if (GET_CONFIG(ENABLE_VOICE))
    {
        // һ�㵯Ļ
        if (cmd == "LIVE_OPEN_PLATFORM_DM")
            TTSManager::Inst()->HandleSpeekDm(jsonResponse["data"]);
        // ����
        else if (cmd == "LIVE_OPEN_PLATFORM_SEND_GIFT")
            TTSManager::Inst()->HandleSpeekSendGift(jsonResponse["data"]);
        // SC
        else if (cmd == "LIVE_OPEN_PLATFORM_SUPER_CHAT")
            TTSManager::Inst()->HandleSpeekSC(jsonResponse["data"]);
        // ����
        else if (cmd == "LIVE_OPEN_PLATFORM_GUARD")
            TTSManager::Inst()->HandleSpeekGuard(jsonResponse["data"]);
    }

    // <TODO> ������C++��Ľ������
    ToolsMainHost::Inst()->OnReceiveRawMsg(decoded);
}