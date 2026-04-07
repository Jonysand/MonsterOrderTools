#include "framework.h"
#include "Network.h"

#ifdef RUN_UNIT_TESTS

namespace Network {
    void MakeHttpsRequestAsync(
        const TString& host,
        const INTERNET_PORT& port,
        const TString& path,
        const TString& method,
        const std::string& headers,
        const std::string& body,
        const bool ssl,
        HttpsAsyncCallback onComplete
    ) {
        if (onComplete) {
            onComplete(true, "", 0);
        }
    }

    void MakeWebSocketConnectionAsync(
        const std::vector<std::string> serverAddresses,
        const std::vector<uint8_t>& authBody,
        WebsocketMessageCallback onMessage,
        WebSocketAsyncCallback onConnect
    ) {
        if (onConnect) {
            onConnect(false, nullptr, ERROR_ACCESS_DENIED);
        }
    }

    void SendToWebsocketAsync(
        HINTERNET hWebsocket,
        const std::vector<uint8_t>& data,
        std::function<void(bool success, DWORD error)> onComplete
    ) {
        if (onComplete) {
            onComplete(false, ERROR_ACCESS_DENIED);
        }
    }
}

#endif
