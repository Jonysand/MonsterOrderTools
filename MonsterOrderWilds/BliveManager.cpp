#include "BliveManager.h"
#include "CredentialsManager.h"
#include "WriteLog.h"
#include "TextToSpeech.h"
#include "DataBridge.h"

DEFINE_SINGLETON(BliveManager)

/*
* 直播连接------------------------------------------------------------------
*/

enum WEBSOCKET_OP {
    OP_UNKNOWN = 0,
    OP_HEARTBEAT = 2,           // 客户端发送的心跳包(30秒发送一次)
    OP_HEARTBEAT_REPLY = 3,	    // 服务器收到心跳包的回复
    OP_SEND_SMS_REPLY = 5,      // 服务器推送的弹幕消息包
    OP_AUTH = 7,               // 客户端发送的鉴权包(客户端发送的第一个包)
    OP_AUTH_REPLY = 8           // 服务器收到鉴权包后的回复
};

void BliveManager::SetConnectionState(ConnectionState state, DisconnectReason reason)
{
    ConnectionState oldState = connectionState.load();
    connectionState.store(state);
    disconnectReason.store(reason);
    
    LOG_INFO(TEXT("Connection state changed: %hs -> %hs, reason: %hs"), 
        ConnectionStateToString(oldState),
        ConnectionStateToString(state),
        DisconnectReasonToString(reason));
    
    OnConnectionStateChanged.Invoke(state, reason);
    
    if (state == ConnectionState::Connected && oldState != ConnectionState::Connected) {
        OnBliveConnected.Invoke();
    }
    else if (oldState == ConnectionState::Connected && state != ConnectionState::Connected) {
        OnBliveDisconnected.Invoke();
    }
}

void BliveManager::Disconnect()
{
    reconnectAttemptCount.store(0);
    
    {
        std::lock_guard<Lock> lock(wsMsgLock);
        if (webSocket) {
            WinHttpCloseHandle(webSocket);
            webSocket = nullptr;
        }
    }
    
    std::string currentGameId;
    {
        std::lock_guard<std::mutex> lock(gameIdLock);
        currentGameId = gameId;
        gameId.clear();
    }
    if (!currentGameId.empty()) {
        End(currentGameId, false);
    }
    
    SetConnectionState(ConnectionState::Disconnected, DisconnectReason::None);
}

void BliveManager::Reconnect()
{
    if (connectionState.load() == ConnectionState::ReconnectFailed)
    {
        reconnectAttemptCount.store(0);
        Start();
    }
    else if (connectionState.load() == ConnectionState::Disconnected)
    {
        Start();
    }
}

void BliveManager::Start(const std::string& IdCode)
{
    if (!IdCode.empty())
        idCode = IdCode;
    if (idCode.empty())
        return;
    
    ConnectionState currentState = connectionState.load();
    if (currentState == ConnectionState::Connecting || currentState == ConnectionState::Connected)
    {
        return;
    }
    
    if (currentState == ConnectionState::Reconnecting || currentState == ConnectionState::ReconnectFailed)
    {
        int currentAttempt = reconnectAttemptCount.load();
        if (currentAttempt >= MAX_RECONNECT_ATTEMPTS)
        {
            LOG_ERROR(TEXT("Max reconnect attempts reached"));
            SetConnectionState(ConnectionState::ReconnectFailed, disconnectReason.load());
            return;
        }
        reconnectAttemptCount.store(currentAttempt + 1);
    }
    else
    {
        reconnectAttemptCount.store(0);
    }
    
    SetConnectionState(ConnectionState::Connecting, DisconnectReason::None);
    
    std::string currentGameId;
    {
        std::lock_guard<std::mutex> lock(gameIdLock);
        currentGameId = gameId;
        gameId.clear();
    }
    if (!currentGameId.empty()) {
        End(currentGameId, true);
        return;
    }
    std::string params = "{\"code\":\"" + idCode + "\",\"app_id\":" + GetAPP_ID() + "}";
    std::string signedHeader = ProtoUtils::Sign(params);

    auto request = std::make_shared<AsyncRequest>();
    request->type = AsyncRequest::HTTP;
    request->httpCallback = [this](bool success, const std::string& response, DWORD error) {
        if (destroying_.load()) return;
        if (success) {
            OnReceiveStartResponse(response);
        } else {
            LOG_ERROR(TEXT("Start request failed with error: %d"), error);
            SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
        }
    };
    {
        std::lock_guard<Lock> lock(networkRequestsLock);
        networkRequests.push_back(request);
    }

    Network::MakeHttpsRequestAsync(
        BLIVE_URL,
        0,
        BLIVE_START_API,
        TEXT("POST"),
        signedHeader,
        params,
        true,
        [request](bool success, const std::string& response, DWORD error) {
            request->Complete(success, response, error);
        }
    );
}

void BliveManager::End(const std::string& GameId, bool restart)
{
    std::string idToUse = GameId.empty() ? [this]() {
        std::lock_guard<std::mutex> lock(gameIdLock);
        return gameId;
    }() : GameId;
    std::string params = "{\"game_id\":\"" + idToUse + "\",\"app_id\":" + GetAPP_ID() + "}";
    std::string signedHeader = ProtoUtils::Sign(params);

    auto request = std::make_shared<AsyncRequest>();
    request->type = AsyncRequest::HTTP;
    request->httpCallback = [this, restart](bool success, const std::string& response, DWORD error) {
        if (destroying_.load()) return;
        if (success) {
            LOG_DEBUG(TEXT("Stop response: %s"), ProtoUtils::Decode(response).c_str());
        }
        if (restart)
            Start();
    };
    {
        std::lock_guard<Lock> lock(networkRequestsLock);
        networkRequests.push_back(request);
    }

    Network::MakeHttpsRequestAsync(
        BLIVE_URL,
        0,
        BLIVE_END_API,
        TEXT("POST"),
        signedHeader,
        params,
        true,
        [request](bool success, const std::string& response, DWORD error) {
            request->Complete(success, response, error);
        }
    );
}

void BliveManager::Tick() {
    // 延时任务
    {
        std::lock_guard<Lock> lock(delayedTasksLock);
        if (!delayedTasks.empty()) {
            auto it = delayedTasks.begin();
            while (it != delayedTasks.end()) {
                if (it->checkInvoke())
                    it = delayedTasks.erase(it);
                else
                    ++it;
            }
        }
    }
    // 处理已完成的异步请求
    {
        std::list<std::function<void()>> callbacksToInvoke;
        {
            std::lock_guard<Lock> lock(networkRequestsLock);
            for (auto it = networkRequests.begin(); it != networkRequests.end(); ) {
                if ((*it)->completed.load()) {
                    auto req = *it;
                    if (req->httpCallback) {
                        bool success = req->error == 0;
                        std::string response = req->response;
                        DWORD error = req->error;
                        callbacksToInvoke.push_back([req, success, response, error]() {
                            try {
                                req->httpCallback(success, response, error);
                            } catch (...) {
                                LOG_ERROR(TEXT("[BliveManager] httpCallback exception"));
                            }
                        });
                    }
                    it = networkRequests.erase(it);
                } else {
                    ++it;
                }
            }
        }
        for (const auto& callback : callbacksToInvoke) {
            callback();
        }
    }
    // websocket 信息
    HandleWSMessage();
}

BliveManager::~BliveManager()
{
    destroying_.store(true);
    {
        std::lock_guard<Lock> lock(wsMsgLock);
        if (webSocket) {
            WinHttpCloseHandle(webSocket);
            webSocket = nullptr;
        }
    }
    {
        std::lock_guard<Lock> lock(networkRequestsLock);
        networkRequests.clear();
    }
    {
        std::lock_guard<Lock> lock(delayedTasksLock);
        delayedTasks.clear();
    }
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
    catch (const json::exception& e) {
		LOG_ERROR(TEXT("JSON error: %s"), ProtoUtils::Decode(e.what()).c_str());
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { Start(); }, 1000 });
        }
        return;
	}
    try {
    switch (code)
    {
    case 0:
    {
        TString decodedGameID = ProtoUtils::Decode(jsonResponse["data"]["game_info"]["game_id"].get<std::string>());
        LOG_INFO(TEXT("Game ID: %s"), decodedGameID.c_str());
        {
            std::lock_guard<std::mutex> lock(gameIdLock);
            gameId = ProtoUtils::ConvertTCharToString(decodedGameID.c_str());
        }
        SetConnectionState(ConnectionState::Connected, DisconnectReason::None);
        // 准备建立websocket
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

        Network::MakeWebSocketConnectionAsync(
            serverAddresses,
            packedData,
            [this](HINTERNET hWebsocket, ProtoUtils::Packet packet) { OnReceiveWSMessage(hWebsocket, std::move(packet)); },
            [this](bool success, HINTERNET websocket, DWORD error) {
                if (!success) {
                    LOG_ERROR(TEXT("WebSocket connection failed with error: %d"), error);
                    SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
                    {
                        std::lock_guard<Lock> lock(delayedTasksLock);
                        delayedTasks.push_back({ [this]() { Start(); }, 1000 });
                    }
                }
            }
        );
        // 开始app心跳
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { StartAppHeartBeat(); }, 5000 });
        }
        break;
    }
    case 7001:
        LOG_ERROR(TEXT("OnReceiveStartResponse: %s"), TEXT("请求冷却期"));
        break;
    default:
        LOG_ERROR(TEXT("Error OnStartResponse: %s"), ProtoUtils::Decode(response).c_str());
        SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { Start(); }, 1000 });
        }
        break;
    }
    } catch (const json::exception& e) {
        LOG_ERROR(TEXT("JSON access error in OnReceiveStartResponse: %s"), ProtoUtils::Decode(e.what()).c_str());
        SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { Start(); }, 1000 });
        }
    }
}

void BliveManager::StartAppHeartBeat()
{
    std::string currentGameId;
    {
        std::lock_guard<std::mutex> lock(gameIdLock);
        currentGameId = gameId;
    }
    if (currentGameId.empty()) {
        LOG_DEBUG(TEXT("StartAppHeartBeat skipped: gameId is empty"));
        return;
    }
    std::string heartbeatParams = "{\"game_id\":\"" + currentGameId + "\"}";
    std::string signedHeartbeatHeader = ProtoUtils::Sign(heartbeatParams);

    auto request = std::make_shared<AsyncRequest>();
    request->type = AsyncRequest::HTTP;
    request->httpCallback = [this](bool success, const std::string& response, DWORD error) {
        if (destroying_.load()) return;
        if (success) {
            OnReceiveAppHeartbeatResponse(response);
        } else {
            LOG_ERROR(TEXT("Heartbeat request failed with error: %d"), error);
            SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
        }
    };
    {
        std::lock_guard<Lock> lock(networkRequestsLock);
        networkRequests.push_back(request);
    }

    Network::MakeHttpsRequestAsync(
        BLIVE_URL,
        0,
        BLIVE_HEARTBEAT_API,
        TEXT("POST"),
        signedHeartbeatHeader,
        heartbeatParams,
        true,
        [request](bool success, const std::string& response, DWORD error) {
            request->Complete(success, response, error);
        }
    );
}

void BliveManager::OnReceiveAppHeartbeatResponse(const std::string& response)
{
    LOG_DEBUG(TEXT("OnReceiveAppHeartbeatResponse: %s"), ProtoUtils::Decode(response).c_str());
    {
        std::lock_guard<std::mutex> lock(gameIdLock);
        if (gameId.empty())
        {
            LOG_DEBUG(TEXT("OnReceiveAppHeartbeatResponse: ignored due to empty gameId"));
            return;
        }
    }
    if (response.empty())
    {
        LOG_ERROR(TEXT("Error OnReceiveAppHeartbeatResponse: empty response"));
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { StartAppHeartBeat(); }, HEARTBEAT_INTERVAL_MINISECONDS });
        }
        return;
    }
    DWORD code = -1;
    json jsonResponse;
    try {
        jsonResponse = json::parse(response);
        if (jsonResponse.contains("code"))
            code = jsonResponse["code"];
        switch (code)
        {
        case 0:
        case 4004:
        {
            reconnectAttemptCount.store(0);
            {
                std::lock_guard<Lock> lock(delayedTasksLock);
                delayedTasks.push_back({ [this]() { StartAppHeartBeat(); }, HEARTBEAT_INTERVAL_MINISECONDS });
            }
            break;
        }
        default:
            SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::HeartbeatTimeout);
            LOG_ERROR(TEXT("Error OnReceiveAppHeartbeatResponse: %s"), ProtoUtils::Decode(response).c_str());
            {
                std::lock_guard<Lock> lock(delayedTasksLock);
                delayedTasks.push_back({ [this]() { Start(); }, 1000 });
            }
            break;
        }
    }
    catch (const json::exception& e) {
        LOG_ERROR(TEXT("JSON error in OnReceiveAppHeartbeatResponse: %s"), ProtoUtils::Decode(e.what()).c_str());
        SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::HeartbeatTimeout);
        {
            std::lock_guard<Lock> lock(delayedTasksLock);
            delayedTasks.push_back({ [this]() { Start(); }, 1000 });
        }
    }
}

void BliveManager::StartWebsocketHeartBeat()
{
    ProtoUtils::Packet packet;
    packet.op = OP_HEARTBEAT;
    std::vector<uint8_t> packedData = packet.pack();

    HINTERNET ws = nullptr;
    {
        std::lock_guard<Lock> lock(wsMsgLock);
        ws = webSocket;
    }

    if (ws) {
        Network::SendToWebsocketAsync(
            ws,
            packedData,
            [this](bool success, DWORD error) {
                if (destroying_.load()) return;
                if (!success) {
                    LOG_ERROR(TEXT("WebSocket heartbeat send failed with error: %d"), error);
                }
            }
        );
    }
}

void BliveManager::OnReceiveWSMessage(HINTERNET hWebsocket, ProtoUtils::Packet packet)
{
    if (connectionState.load() != ConnectionState::Connected)
        return;
    std::lock_guard<Lock> lock(wsMsgLock);
    webSocket = hWebsocket;
    wsMessages.push(packet);
}

void BliveManager::HandleWSMessage()
{
    uint8_t count = 0;
    while (count < 10)
    {
        ProtoUtils::Packet packet;
        {
            std::lock_guard<Lock> lock(wsMsgLock);
            if (wsMessages.empty())
                break;
            packet = wsMessages.front();
            wsMessages.pop();
        }
        WEBSOCKET_OP op = (WEBSOCKET_OP)packet.op;
        switch (op)
        {
        case OP_HEARTBEAT:
            LOG_ERROR(TEXT("Receive OP_HEARTBEAT from server websocket"));
            break;
        case OP_HEARTBEAT_REPLY:
            reconnectAttemptCount.store(0);
            {
                std::lock_guard<Lock> lock(delayedTasksLock);
                delayedTasks.push_back({ [this]() { StartWebsocketHeartBeat(); }, HEARTBEAT_INTERVAL_MINISECONDS });
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
            {
                std::lock_guard<Lock> lock(delayedTasksLock);
                delayedTasks.push_back({ [this]() { StartWebsocketHeartBeat(); }, 2000 });
            }
            break;
        default:
            SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::NetworkError);
            LOG_ERROR(TEXT("Receive UNKNOWN from server websocket: %s"), ProtoUtils::Decode(packet.body).c_str());
            {
                std::lock_guard<Lock> lock(delayedTasksLock);
                delayedTasks.push_back({ [this]() { Start(); }, 1000 });
            }
            break;
        }
        ++count;
    }
}

void BliveManager::HandleSmsReply(const std::string& msg)
{
    try {
        TString decoded = ProtoUtils::Decode(msg);
        LOG_INFO(TEXT("OP_SEND_SMS_REPLY: %s"), decoded.c_str());

        json jsonResponse;
        try {
            jsonResponse = json::parse(msg);
        }
        catch (const json::parse_error&) {
            LOG_ERROR(TEXT("[HandleSmsReply] JSON parse error: %s"), msg.c_str());
            return;
        }
        const std::string& cmd = jsonResponse["cmd"].get<std::string>();
        // 一般弹幕
        if (cmd == "LIVE_OPEN_PLATFORM_DM")
            TTSManager::Inst()->HandleSpeekDm(jsonResponse["data"]);
        // 送礼
        else if (cmd == "LIVE_OPEN_PLATFORM_SEND_GIFT")
            TTSManager::Inst()->HandleSpeekSendGift(jsonResponse["data"]);
        // SC
        else if (cmd == "LIVE_OPEN_PLATFORM_SUPER_CHAT")
            TTSManager::Inst()->HandleSpeekSC(jsonResponse["data"]);
        // 舰长
        else if (cmd == "LIVE_OPEN_PLATFORM_GUARD")
            TTSManager::Inst()->HandleSpeekGuard(jsonResponse["data"]);
        // 进入房间
        else if (cmd == "LIVE_OPEN_PLATFORM_LIVE_ROOM_ENTER")
            TTSManager::Inst()->HandleSpeekEnter(jsonResponse["data"]);
        // 断连，直接重连
        else if (cmd == "LIVE_OPEN_PLATFORM_INTERACTION_END")
        {
            const std::string& disconnectedGameId = jsonResponse["data"]["game_id"].get<std::string>();
            {
                std::lock_guard<std::mutex> lock(gameIdLock);
                if (disconnectedGameId == gameId)
                {
                    gameId.clear();
                    SetConnectionState(ConnectionState::Reconnecting, DisconnectReason::ServerClose);
                    {
                        std::lock_guard<Lock> lock(delayedTasksLock);
                        delayedTasks.push_back({ [this]() { Start(); }, 1000 });
                    }
                }
            }
        }

        if (cmd == "LIVE_OPEN_PLATFORM_DM") {
            DanmuProcessor::Inst()->ProcessDanmu(DanmuProcessor::Inst()->ParseDanmuJson(jsonResponse));
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(TEXT("[HandleSmsReply] Exception: %s"), ProtoUtils::Decode(e.what()).c_str());
    }
    catch (...) {
        LOG_ERROR(TEXT("[HandleSmsReply] Unknown exception occurred"), TEXT(""));
    }
}