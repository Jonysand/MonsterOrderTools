#include "Network.h"
#include "BliveManager.h"
#include "CredentialsManager.h"
#include "WriteLog.h"
#include <mutex>
#include <thread>
#include <condition_variable>

// Protoco Utils
namespace ProtoUtils
{
    // https的body必须是string，或者说其单位字符大小必须是sizeof(char)，不然服务端接收header中的Content-Length和实际body大小会不匹配
    std::string ConvertTCharToString(const TCHAR* tcharStr) {
        if (tcharStr == nullptr) {
            return std::string();
        }
#ifdef UNICODE
        // Convert wide string (TCHAR as wchar_t) to std::string
        std::wstring wideStr(tcharStr);
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Length == 0) {
            throw std::runtime_error("WideCharToMultiByte failed: " + std::to_string(GetLastError()));
        }
        std::string utf8Str(utf8Length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Length, nullptr, nullptr);
        utf8Str.pop_back(); // Remove null terminator
        return utf8Str;
#else
        // TCHAR is char, directly convert to std::string
        return std::string(tcharStr);
#endif
    }
    TString ConvertToTCHAR(const char* input) {
        if (!input) {
            return TString();
        }

#ifdef UNICODE
        // Convert from char* to std::wstring using MultiByteToWideChar
        int wideLength = MultiByteToWideChar(CP_UTF8, 0, input, -1, nullptr, 0);
        if (wideLength == 0) {
            return TString();
        }
        std::wstring wideStr(wideLength - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, input, -1, &wideStr[0], wideLength);
        return wideStr;
#else
        // Directly return std::string in ANSI builds
        return std::string(input);
#endif
    }
    template <typename T>
    std::string Encode(const T& input)
    {
        static_assert(std::is_same<T, std::string>::value || std::is_same<T, std::wstring>::value,
            "Encode only supports std::string or std::wstring as input.");
        std::wstring wideStr;
        if constexpr (std::is_same<T, std::wstring>::value)
        {
            // Input is already a wide string
            wideStr = input;
        }
        else
        {
            // Convert std::string to std::wstring
            int wideLength = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, nullptr, 0);
            if (wideLength == 0)
            {
                throw std::runtime_error("MultiByteToWideChar failed: " + std::to_string(GetLastError()));
            }
            wideStr.resize(wideLength - 1); // Exclude null terminator
            MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, &wideStr[0], wideLength);
        }
        // Convert std::wstring to UTF-8 std::string
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Length == 0)
        {
            throw std::runtime_error("WideCharToMultiByte failed: " + std::to_string(GetLastError()));
        }
        std::string utf8Str(utf8Length - 1, '\0'); // Exclude null terminator
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Length, nullptr, nullptr);
        return utf8Str;
    }
    TString Decode(const std::string& params)
    {
        int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, params.c_str(), -1, nullptr, 0);
        if (wideCharLength == 0) {
            throw std::runtime_error("MultiByteToWideChar failed: " + std::to_string(GetLastError()));
        }
        std::wstring decodedResponse(wideCharLength, L'\0');
        if (MultiByteToWideChar(CP_UTF8, 0, params.c_str(), -1, &decodedResponse[0], wideCharLength) == 0) {
            throw std::runtime_error("MultiByteToWideChar failed: " + std::to_string(GetLastError()));
        }
        // Remove the null terminator added by MultiByteToWideChar
        decodedResponse.pop_back();
        return decodedResponse;
    }

    // AI写代码真好用
#pragma comment(lib, "bcrypt.lib")
// 弹幕姬把这一步部署在自己的服务器上，之后也可以这么做，不用代码或者本地存key
    std::string Sign(const std::string& params)
    {
        // md5
        std::string md5data = hashpp::get::getHash(hashpp::ALGORITHMS::MD5, ProtoUtils::Encode(params));

        // Generate timestamp and nonce
        std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
        std::time_t ts = std::time(nullptr);
        std::uniform_int_distribution<int> dist(1, 100000);
        std::string nonce = std::to_string(dist(rng)) + std::to_string(ts);

        // Create header map
        std::map<std::string, std::string> headerMap = {
            {"x-bili-timestamp", std::to_string(ts)},
            {"x-bili-signature-method", "HMAC-SHA256"},
            {"x-bili-signature-nonce", nonce},
            {"x-bili-accesskeyid", GetACCESS_KEY_ID()},
            {"x-bili-signature-version", "1.0"},
            {"x-bili-content-md5", md5data}
        };

        // Sort headers and construct header string
        std::ostringstream headerStrStream;
        for (const auto& [key, value] : headerMap) {
            headerStrStream << key << ":" << value << "\n";
        }
        std::string headerStr = headerStrStream.str();
        headerStr.pop_back(); // Remove trailing newline

        // Generate HMAC-SHA256 signature
        std::string signature = hashpp::get::getHMAC(hashpp::ALGORITHMS::SHA2_256, ProtoUtils::Encode(GetACCESS_KEY_SECRET()), ProtoUtils::Encode(headerStr));

        // Add signature and other headers
        headerMap["Authorization"] = signature;
        headerMap["Content-Type"] = "application/json";

        // Convert header map to JSON-like string (or any required format)
        std::ostringstream resultStream;
        for (const auto& [key, value] : headerMap) {
            // resultStream << "\"" << key << "\":\"" << value << "\",";
            resultStream << key << ":" << value << "\n";
        }
        std::string result = resultStream.str();
        result.pop_back();

        return result;
    }
}

namespace Network
{
    namespace HttpsAsyncUtils {
        struct HttpsAsyncContext {
            HINTERNET hConnect = nullptr;
            HINTERNET hRequest = nullptr;
            std::string response;
            DWORD error = 0;
            Network::HttpsAsyncCallback callback;
            bool completed = false;
            std::mutex mtx;
            std::atomic<bool> cleanupDone{false};
        };

        void CALLBACK HttpsStatusCallback(
            HINTERNET hInternet,
            DWORD_PTR dwContext,
            DWORD internetStatus,
            LPVOID statusInfo,
            DWORD statusInfoLength
        ) {
            auto* ctx = reinterpret_cast<HttpsAsyncContext*>(dwContext);
            if (!ctx) return;
            switch (internetStatus) {
            case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
                {
                    std::lock_guard<std::mutex> lock(ctx->mtx);
                    if (statusInfoLength >= sizeof(WINHTTP_ASYNC_RESULT)) {
                        ctx->error = ((WINHTTP_ASYNC_RESULT*)statusInfo)->dwError;
                    } else {
                        ctx->error = GetLastError();
                    }
                    ctx->completed = true;
                }
                std::thread([ctx]() {
                    if (ctx->cleanupDone.exchange(true)) return;
                    std::lock_guard<std::mutex> lock(ctx->mtx);
                    if (ctx->callback) {
                        try {
                            ctx->callback(false, ctx->response, ctx->error);
                        } catch (...) {
                        }
                    }
                    if (ctx->hRequest) WinHttpCloseHandle(ctx->hRequest);
                    if (ctx->hConnect) WinHttpCloseHandle(ctx->hConnect);
                    delete ctx;
                }).detach();
                break;
            case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
                {
                    std::lock_guard<std::mutex> lock(ctx->mtx);
                    ctx->completed = true;
                }
                break;
            default:
                break;
            }
        }

        DWORD ReadResponseData(HttpsAsyncContext* ctx) {
            DWORD totalBytesRead = 0;
            while (true) {
                DWORD bytesAvailable = 0;
                if (!WinHttpQueryDataAvailable(ctx->hRequest, &bytesAvailable) || bytesAvailable == 0)
                    break;
                std::vector<char> buffer(bytesAvailable);
                DWORD bytesRead = 0;
                if (!WinHttpReadData(ctx->hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                    ctx->error = GetLastError();
                    return ctx->error;
                }
                if (bytesRead > 0)
                    ctx->response.append(buffer.data(), bytesRead);
                if (bytesRead < bytesAvailable)
                    break;
            }
            return 0;
        }

        HINTERNET GetSharedSession() {
            static HINTERNET hSession = nullptr;
            static std::once_flag flag;
            std::call_once(flag, []() {
                hSession = WinHttpOpen(HTTP_REQUEST_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
                if (hSession) {
                    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);
                }
            });
            return hSession;
        }
    }

    void MakeHttpsRequestAsync(
        const TString& host,
        const INTERNET_PORT& port,
        const TString& path,
        const TString& method,
        const std::string& headers,
        const std::string& body,
        const bool ssl,
        Network::HttpsAsyncCallback onComplete
    ) {
        auto* ctx = new HttpsAsyncUtils::HttpsAsyncContext();
        ctx->callback = onComplete;

        HINTERNET hSession = HttpsAsyncUtils::GetSharedSession();
        if (!hSession) {
            ctx->error = GetLastError();
            std::thread([ctx]() {
                if (ctx->callback) {
                    try {
                        ctx->callback(false, "", ctx->error);
                    } catch (...) {
                    }
                }
                delete ctx;
            }).detach();
            return;
        }

        INTERNET_PORT realPort = port == 0 ? (ssl ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_PORT) : port;
        ctx->hConnect = WinHttpConnect(hSession, host.c_str(), realPort, 0);
        if (!ctx->hConnect) {
            ctx->error = GetLastError();
            std::thread([ctx]() {
                if (ctx->callback) {
                    try {
                        ctx->callback(false, "", ctx->error);
                    } catch (...) {
                    }
                }
                delete ctx;
            }).detach();
            return;
        }

        LPCTSTR szAccept[] = { TEXT("application/json"), NULL };
        DWORD FLAG = ssl ? WINHTTP_FLAG_SECURE : 0;
        ctx->hRequest = WinHttpOpenRequest(ctx->hConnect, method.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, szAccept, FLAG);
        if (!ctx->hRequest) {
            ctx->error = GetLastError();
            std::thread([ctx]() {
                if (ctx->callback) {
                    try {
                        ctx->callback(false, "", ctx->error);
                    } catch (...) {
                    }
                }
                WinHttpCloseHandle(ctx->hConnect);
                delete ctx;
            }).detach();
            return;
        }

        WinHttpSetStatusCallback(ctx->hRequest, HttpsAsyncUtils::HttpsStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
        WinHttpSetOption(ctx->hRequest, WINHTTP_OPTION_CONTEXT_VALUE, &ctx, sizeof(ctx));

        std::thread([ctx, headers, body]() {
            if (!headers.empty()) {
                TString tcharHeaders = ProtoUtils::ConvertToTCHAR(headers.c_str());
                WinHttpAddRequestHeaders(ctx->hRequest, tcharHeaders.c_str(), (DWORD)tcharHeaders.size(), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
            }

            BOOL sendResult = WinHttpSendRequest(
                ctx->hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
                (DWORD)body.size(),
                (DWORD)body.size(),
                reinterpret_cast<DWORD_PTR>(ctx)
            );

            if (!sendResult) {
                ctx->error = GetLastError();
                ctx->completed = true;
            } else {
                BOOL receiveResult = WinHttpReceiveResponse(ctx->hRequest, NULL);
                if (!receiveResult) {
                    ctx->error = GetLastError();
                    ctx->completed = true;
                } else {
                    ctx->error = HttpsAsyncUtils::ReadResponseData(ctx);
                    ctx->completed = true;
                }
            }

            {
                if (ctx->cleanupDone.exchange(true)) return;
                std::lock_guard<std::mutex> lock(ctx->mtx);
                if (ctx->callback) {
                    try {
                        ctx->callback(ctx->error == 0, ctx->response, ctx->error);
                    } catch (...) {
                    }
                }
                if (ctx->hRequest) WinHttpCloseHandle(ctx->hRequest);
                if (ctx->hConnect) WinHttpCloseHandle(ctx->hConnect);
                delete ctx;
            }
        }).detach();
    }

    void StartWebSocketReceive(HINTERNET hWebSocket, Network::WebsocketMessageCallback onMessage);

    void MakeWebSocketConnectionAsync(
        const std::vector<std::string> serverAddresses,
        const std::vector<uint8_t>& authBody,
        Network::WebsocketMessageCallback onMessage,
        Network::WebSocketAsyncCallback onConnect
    ) {
        std::thread([serverAddresses, authBody, onMessage, onConnect]() {
            HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr, hWebSocket = nullptr;

            for (const auto& serverAddress : serverAddresses) {
                URL_COMPONENTS urlComponents = { 0 };
                urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
                WCHAR hostName[256] = { 0 };
                WCHAR urlPath[256] = { 0 };
                WCHAR scheme[16] = { 0 };
                urlComponents.lpszHostName = hostName;
                urlComponents.dwHostNameLength = ARRAYSIZE(hostName);
                urlComponents.lpszUrlPath = urlPath;
                urlComponents.dwUrlPathLength = ARRAYSIZE(urlPath);
                urlComponents.lpszScheme = scheme;
                urlComponents.dwSchemeLength = ARRAYSIZE(scheme);
                std::wstring wideServerAddress = ProtoUtils::ConvertToTCHAR(serverAddress.c_str());
                if (wideServerAddress.find(L"wss://") == 0)
                    wideServerAddress.replace(0, 6, L"https://");
                if (!WinHttpCrackUrl(wideServerAddress.c_str(), 0, ICU_DECODE, &urlComponents)) {
                    LOG_ERROR(TEXT("Failed to parse URL: %s, Error: %d"), ProtoUtils::ConvertToTCHAR(serverAddress.c_str()).c_str(), GetLastError());
                    continue;
                }

                hSession = WinHttpOpen(WEBSOCKET_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                if (!hSession) {
                    LOG_ERROR(TEXT("WinHttpOpen failed: %d"), GetLastError());
                    continue;
                }

                hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, urlComponents.nPort, 0);
                if (!hConnect) {
                    LOG_ERROR(TEXT("WinHttpConnect failed: %d"), GetLastError());
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComponents.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (!hRequest) {
                    LOG_ERROR(TEXT("WinHttpOpenRequest failed: %d"), GetLastError());
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0)) {
                    LOG_ERROR(TEXT("WinHttpSetOption failed: %d"), GetLastError());
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                    LOG_ERROR(TEXT("WinHttpSendRequest failed: %d"), GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                if (!WinHttpReceiveResponse(hRequest, NULL)) {
                    LOG_ERROR(TEXT("WinHttpReceiveResponse failed: %d"), GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                DWORD statusCode = 0;
                DWORD statusCodeSize = sizeof(statusCode);
                if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX)) {
                    LOG_ERROR(TEXT("Failed to query status code: %d"), GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }
                if (statusCode != 101) {
                    LOG_ERROR(TEXT("WebSocket handshake failed with status code: %s, %d"), ProtoUtils::ConvertToTCHAR(serverAddress.c_str()).c_str(), statusCode);
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, 0);
                if (!hWebSocket) {
                    LOG_ERROR(TEXT("WinHttpWebSocketCompleteUpgrade failed: %d"), GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    continue;
                }

                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);

                if (WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void*)authBody.data(), (DWORD)authBody.size()) != ERROR_SUCCESS) {
                    LOG_ERROR(TEXT("WinHttpWebSocketSend auth failed: %d"), GetLastError());
                    WinHttpCloseHandle(hWebSocket);
                    if (onConnect) {
                        try {
                            onConnect(false, nullptr, GetLastError());
                        } catch (...) {
                        }
                    }
                    return;
                }

                StartWebSocketReceive(hWebSocket, onMessage);
                if (onConnect) {
                    try {
                        onConnect(true, hWebSocket, 0);
                    } catch (...) {
                    }
                }
                return;
            }

            LOG_ERROR(TEXT("Failed to establish WebSocket connection to any server."));
            if (onConnect) {
                try {
                    onConnect(false, nullptr, ERROR_CONNECTION_REFUSED);
                } catch (...) {
                }
            }
        }).detach();
    }

    void SendToWebsocketAsync(
        HINTERNET hWebsocket,
        const std::vector<uint8_t>& data,
        std::function<void(bool success, DWORD error)> onComplete
    ) {
        std::thread([hWebsocket, data, onComplete]() {
            DWORD result = WinHttpWebSocketSend(hWebsocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void*)data.data(), (DWORD)data.size());
            if (result != ERROR_SUCCESS) {
                LOG_ERROR(TEXT("WinHttpWebSocketSend failed: %d"), result);
                if (onComplete) {
                    try {
                        onComplete(false, result);
                    } catch (...) {
                    }
                }
                return;
            }
            if (onComplete) {
                try {
                    onComplete(true, 0);
                } catch (...) {
                }
            }
        }).detach();
    }

    void StartWebSocketReceive(HINTERNET hWebSocket, Network::WebsocketMessageCallback onMessage) {
        std::thread([hWebSocket, onMessage]() {
            while (BliveManager::Inst()->IsConnected()) {
                char headerBuffer[16];
                ProtoUtils::Packet onePacket;
                DWORD bytesTransferred = 0;
                WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

                DWORD result = WinHttpWebSocketReceive(hWebSocket, headerBuffer, 16, &bytesTransferred, &bufferType);
                if (result == ERROR_SUCCESS) {
                    if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                        LOG_ERROR(TEXT("WebSocket connection closed by the server."));
                        return;
                    }
                    if (bytesTransferred != 16) {
                        LOG_ERROR(TEXT("Incomplete WebSocket header: %d/16 bytes"), bytesTransferred);
                        continue;
                    }
                    if (!onePacket.unpackHeader((uint8_t*)headerBuffer))
                        continue;
                }
                auto bodyLength = onePacket.packetLen - 16;
                if (bodyLength <= 0)
                    continue;
                onePacket.body.resize(bodyLength);
                result = WinHttpWebSocketReceive(hWebSocket, onePacket.body.data(), bodyLength, &bytesTransferred, &bufferType);
                if (result == ERROR_SUCCESS) {
                    if (bytesTransferred == bodyLength && onMessage) {
                        try {
                            onMessage(hWebSocket, std::move(onePacket));
                        } catch (...) {
                        }
                    }
                }
                else if (result == ERROR_WINHTTP_OPERATION_CANCELLED) {
                    LOG_ERROR(TEXT("WebSocket receive operation cancelled."));
                    break;
                }
                else {
                    LOG_ERROR(TEXT("WinHttpWebSocketReceive failed: %d"), result);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }).detach();
    }
}