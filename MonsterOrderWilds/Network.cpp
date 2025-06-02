#include "Network.h"
#include "CredentialsConsts.h"
#include "WriteLog.h"

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
    TCHAR* ConvertToTCHAR(const char* input) {
        if (!input) {
            return nullptr;
        }

#ifdef UNICODE
        // Convert from char* to wchar_t*
        size_t len = strlen(input) + 1;
        size_t converted = 0;
        wchar_t* wideStr = new wchar_t[len];
        mbstowcs_s(&converted, wideStr, len, input, _TRUNCATE);
        return wideStr;
#else
        // Directly return the char* as TCHAR* in ANSI builds
        size_t len = strlen(input) + 1;
        char* ansiStr = new char[len];
        strcpy_s(ansiStr, len, input);
        return ansiStr;
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
            {"x-bili-accesskeyid", credentials::ACCESS_KEY_ID},
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
        std::string signature = hashpp::get::getHMAC(hashpp::ALGORITHMS::SHA2_256, ProtoUtils::Encode(credentials::ACCESS_KEY_SECRET), ProtoUtils::Encode(headerStr));

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


namespace Network {
    // MakeHttpsRequest
    NetworkCoroutine MakeHttpsRequest(
        const TString& host,
        const INTERNET_PORT& port,
        const TString& path,
        const TString& method,
        const std::string& headers,
        const std::string& body,
        const bool ssl,
        HTTPRequstCallback onResponse
    ) {
        HINTERNET hSession = WinHttpOpen(HTTP_REQUEST_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession)
        {
            LOG_ERROR(TEXT("InternetOpen failed: %d"), GetLastError());
            if (onResponse) onResponse("");
            throw std::runtime_error("InternetOpen failed: " + std::to_string(GetLastError()));
        }
        // set 5 seconds timeout
        if (!WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000)) {
            LOG_ERROR(TEXT("WinHttpSetTimeouts failed: %d"), GetLastError());
            if (onResponse) onResponse("");
            WinHttpCloseHandle(hSession);
            throw std::runtime_error("WinHttpSetTimeouts failed: " + std::to_string(GetLastError()));
        }
        INTERNET_PORT realPort = port == 0 ? (ssl ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_PORT) : port;
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), realPort, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            LOG_ERROR(TEXT("InternetConnect failed: %s"), GetLastErrorAsTString().c_str());
            if (onResponse) onResponse("");
            throw std::runtime_error("InternetConnect failed: " + std::to_string(GetLastError()));
        }

        LPCTSTR szAccept[] = { TEXT("application/json"), NULL };
        DWORD FLAG = ssl ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, szAccept, FLAG);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            LOG_ERROR(TEXT("HttpOpenRequest failed: %s"), GetLastErrorAsTString().c_str());
            if (onResponse) onResponse("");
            throw std::runtime_error("HttpOpenRequest failed: " + std::to_string(GetLastError()));
        }

        const TString bodysTString = TString(body.begin(), body.end());
        if (!headers.empty())
        {
            if (!WinHttpAddRequestHeaders(hRequest, ProtoUtils::ConvertToTCHAR(headers.c_str()), (DWORD)headers.size(), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
            {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                LOG_ERROR(TEXT("HttpAddRequestHeaders failed: %s"), GetLastErrorAsTString().c_str());
                if (onResponse) onResponse("");
                throw std::runtime_error("HttpAddRequestHeaders failed: " + std::to_string(GetLastError()));
            }
        }
        
        co_await HttpsCorouineUtils::SendRequestAsync(hRequest, body.empty() ? NULL : bodysTString.c_str());
        std::string response = co_await HttpsCorouineUtils::ReadResponseAsync(hRequest);
        if (onResponse) onResponse(response);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    }

    void StartWebSocketReceive(HINTERNET hWebSocket, WebsocketMessageCallback onMessage) {
        std::thread([hWebSocket, onMessage]() {
            while (true) {
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
                    if (!onePacket.unpackHeader((uint8_t*)headerBuffer))
                        continue;
                }
                auto bodyLength = onePacket.packetLen - 16;
                if (bodyLength == 0)
                    continue;
                onePacket.body.resize(bodyLength);
                result = WinHttpWebSocketReceive(hWebSocket, onePacket.body.data(), bodyLength, &bytesTransferred, &bufferType);
                if (result == ERROR_SUCCESS) {
                    if (bytesTransferred > 0 && onMessage) {
                        // Pass the received message to the callback
                        onMessage(hWebSocket, std::move(onePacket));
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
                // Optional: Add a small delay to prevent busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }).detach();
    }

    NetworkCoroutine MakeWebSocketConnection(
        const std::vector<std::string> serverAddresses,
        const std::vector<uint8_t>& authBody,
        WebsocketMessageCallback onMessage
    ) {
        HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr, hWebSocket = nullptr;

        for (const auto& serverAddress : serverAddresses) {
            // WinHttpCrackUrl
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
            if (wideServerAddress.find(L"wss://") == 0) {
                wideServerAddress.replace(0, 6, L"https://");
            }
            if (!WinHttpCrackUrl(wideServerAddress.c_str(), 0, ICU_DECODE, &urlComponents)) {
                LOG_ERROR(TEXT("Failed to parse URL: %s, Error: %d"), ProtoUtils::ConvertToTCHAR(serverAddress.c_str()), GetLastError());
                continue;
            }

            // Open a WinHTTP session
            hSession = WinHttpOpen(WEBSOCKET_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                LOG_ERROR(TEXT("WinHttpOpen failed: %d"), GetLastError());
                continue;
            }

            // Connect to the server
            hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, urlComponents.nPort, 0);
            if (!hConnect) {
                LOG_ERROR(TEXT("WinHttpConnect failed: %d"), GetLastError());
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Create an HTTP request handle
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

            // Send Handshake
            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                LOG_ERROR(TEXT("WinHttpSendRequest failed: %d"), GetLastError());
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Receive Handshake response
            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                LOG_ERROR(TEXT("WinHttpReceiveResponse failed: %d"), GetLastError());
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Query the status code
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX)) {
                LOG_ERROR(TEXT("Failed to query status code: %d"), GetLastError());
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            // Check if the status code indicates success
            if (statusCode != 101) { // 101 is the expected status code for WebSocket upgrade
                LOG_ERROR(TEXT("WebSocket handshake failed with status code: %s, %d"), ProtoUtils::ConvertToTCHAR(serverAddress.c_str()), statusCode);
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Upgrade the connection to a WebSocket
            hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, 0);
            if (!hWebSocket) {
                LOG_ERROR(TEXT("WinHttpWebSocketCompleteUpgrade failed: %d"), GetLastError());
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Send the authentication body
            if (WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void*)authBody.data(), (DWORD)authBody.size()) != ERROR_SUCCESS) {
                LOG_ERROR(TEXT("WinHttpWebSocketSend auth failed: %d"), GetLastError());
                WinHttpCloseHandle(hWebSocket);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }

            // Read WebSocket messages
            StartWebSocketReceive(hWebSocket, onMessage);
            while (true) {
                co_await std::suspend_always();
            }

            // Clean up
            WinHttpCloseHandle(hWebSocket);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            co_return;
        }

        LOG_ERROR(TEXT("Failed to establish WebSocket connection to any server."));
        co_return;
    }

    NetworkCoroutine SendToWebsocket(
        HINTERNET hWebsocket,
        const std::vector<uint8_t>& data
    ) {
        DWORD result = WinHttpWebSocketSend(hWebsocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, (void*)data.data(), (DWORD)data.size());
        if (result != ERROR_SUCCESS) {
            LOG_ERROR(TEXT("WinHttpWebSocketSend failed: %d"), result);
            co_return;
        }
        co_return;
    }

    // private utils ------------------------------------
    namespace HttpsCorouineUtils
    {
        // 异步发送请求
        HttpsAwaiter SendRequestAsync(HINTERNET hRequest, const TCHAR* body) {
            HttpsAwaiter awaiter{ hRequest };
            std::string bodyContent = ProtoUtils::ConvertTCharToString(body);
            if (!WinHttpSendRequest(
                hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                bodyContent.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)bodyContent.data(),
                (DWORD)bodyContent.size(),
                (DWORD)bodyContent.size(),
                0)) {
                LOG_ERROR(TEXT("HttpSendRequest failed: %s"), GetLastErrorAsTString().c_str());
            }
            return awaiter;
        }

        // 异步读取响应
        HttpsAwaiter ReadResponseAsync(HINTERNET hRequest) {
            HttpsAwaiter awaiter{ hRequest };

            DWORD bytesAvailable = 0;
            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                LOG_ERROR(TEXT("ReadResponseAsync WinHttpReceiveResponse failed: %d"), GetLastError());
                return awaiter;
            }
            do {
                if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                    LOG_ERROR(TEXT("WinHttpQueryDataAvailable failed: %d"), GetLastError());
                    break;
                }
                if (bytesAvailable > 0) {
                    std::vector<char> buffer(bytesAvailable);
                    DWORD bytesRead = 0;
                    if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                        LOG_ERROR(TEXT("WinHttpReadData failed: %d"), GetLastError());
                    }
                    awaiter.result.append(buffer.data(), bytesRead);
                }
            } while (bytesAvailable > 0);
            return awaiter;
        }
    }
}