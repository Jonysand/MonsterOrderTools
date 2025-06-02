#pragma once
#include "framework.h"

#define HTTP_REQUEST_USER_AGENT TEXT("MonsterOrderWilds Request/0.1")
#define WEBSOCKET_USER_AGENT TEXT("MonsterOrderWilds Websocket/0.1")


#define HEARBEAT_INTERVAL_MINISECONDS 5000 //

// consts
const static TString BLIVE_URL = TEXT("live-open.biliapi.com");
const static TString BLIVE_START_API = TEXT("/v2/app/start");
const static TString BLIVE_END_API = TEXT("/v2/app/end");
const static TString BLIVE_HEARTBEAT_API = TEXT("/v2/app/heartbeat");

const static TString JONYSAND_URL = TEXT("");


// Protoco Utils
namespace ProtoUtils
{
    // https的body必须是string，或者说其单位字符大小必须是sizeof(char)，不然服务端接收header中的Content-Length和实际body大小会不匹配
    std::string ConvertTCharToString(const TCHAR* tcharStr);
    template <typename T>
    std::string Encode(const T& input);
    TString Decode(const std::string& params);
    // 签名
    std::string Sign(const std::string& params);
    struct Packet {
        int32_t packetLen = 0;
        int16_t headerLen = 16;
        int16_t ver = 0;
        int32_t op = 0;
        int32_t seq = 0;
        std::string body = "";
        static const uint16_t maxBody = 2048;

        std::vector<uint8_t> pack() {
            // Calculate the total packet length
            packetLen = static_cast<int32_t>(body.size()) + headerLen;
            // Create a buffer to hold the packed data
            std::vector<uint8_t> buf;
            buf.reserve(packetLen);
            // Pack the fields into the buffer
            appendToBuffer(buf, packetLen);
            appendToBuffer(buf, headerLen);
            appendToBuffer(buf, ver);
            appendToBuffer(buf, op);
            appendToBuffer(buf, seq);
            // Append the body
            buf.insert(buf.end(), body.begin(), body.end());
            return buf;
        }
        bool unpackHeader(uint8_t* buf) {
            // Unpack fields from the buffer
            packetLen = fromBigEndian<int32_t>(buf);
            headerLen = fromBigEndian<int16_t>(buf + 4);
            ver = fromBigEndian<int16_t>(buf + 6);
            op = fromBigEndian<int32_t>(buf + 8);
            seq = fromBigEndian<int32_t>(buf + 12);
            if (packetLen < 0 || packetLen > maxBody) {
                std::cerr << "Invalid packet length: " << packetLen
                    << ", maxBody: " << maxBody << std::endl;
                return false;
            }
            if (headerLen != this->headerLen) {
                std::cerr << "Invalid header length" << std::endl;
                return false;
            }
            int bodyLen = packetLen - headerLen;
            if (bodyLen <= 0) {
                return true; // No body to process
            }
            return true;
        }

    private:
        // Helper function to append data to the buffer in big-endian format
        template <typename T>
        void appendToBuffer(std::vector<uint8_t>& buf, T value) {
            T bigEndianValue = toBigEndian(value);
            uint8_t* data = reinterpret_cast<uint8_t*>(&bigEndianValue);
            buf.insert(buf.end(), data, data + sizeof(T));
        }
        // Convert a value to big-endian format
        template <typename T>
        T toBigEndian(T value) {
            if constexpr (sizeof(T) == 2) {
                return static_cast<T>((value >> 8) | (value << 8));
            }
            else if constexpr (sizeof(T) == 4) {
                return static_cast<T>(((value >> 24) & 0xFF) |
                    ((value >> 8) & 0xFF00) |
                    ((value << 8) & 0xFF0000) |
                    ((value << 24) & 0xFF000000));
            }
            else {
                return value; // No conversion needed for other sizes
            }
        }
        // Helper function to convert from big-endian to host-endian
        template <typename T>
        T fromBigEndian(const uint8_t* data) {
            T value = 0;
            for (size_t i = 0; i < sizeof(T); ++i) {
                value = (value << 8) | data[i];
            }
            return value;
        }
    };
}

namespace Network {
    // 网络协程
    struct NetworkCoroutine {
        struct promise_type {
            static NetworkCoroutine get_return_object_on_allocation_failure() noexcept { return NetworkCoroutine{ nullptr }; }
            NetworkCoroutine get_return_object() { return NetworkCoroutine{ handle::from_promise(*this) }; }
            std::suspend_never initial_suspend() {  return {}; }
            std::suspend_always final_suspend() noexcept {
                // must suspend
                // or done() still return false and crash on resume();
                return {};
            }
            void unhandled_exception() {
                // std::terminate(); 
            }
            void return_void() {}
        };
        using handle = std::coroutine_handle<promise_type>;
        NetworkCoroutine() : coro(nullptr) { }
        NetworkCoroutine(NetworkCoroutine&& rhs) noexcept : coro(rhs.coro) {
            rhs.coro = nullptr;
        }
        ~NetworkCoroutine() {
            if (coro) {
                coro.destroy();
            }
        }
        bool resume() {
            if (!coro || coro.done())
                return false;
            coro.resume();
            return true;
        }
    private:
        explicit NetworkCoroutine(handle h) : coro(h) {}
        handle coro;
    };

    using HTTPRequstCallback = std::function<void(const std::string&)>;
    // HTTP请求
    NetworkCoroutine MakeHttpsRequest(
        const TString& host,
        const INTERNET_PORT& port,
        const TString& path,
        const TString& method,
        const std::string& headers = "",
        const std::string& body = "",
        const bool ssl = true,
        HTTPRequstCallback onResponse = NULL
    );

    using WebsocketMessageCallback = std::function<void(HINTERNET hWebsocket, ProtoUtils::Packet)>;
    // 建立弹幕服务器长链接
    NetworkCoroutine MakeWebSocketConnection(
        const std::vector<std::string> serverAddresses,
        const std::vector<uint8_t>& authBody,
        WebsocketMessageCallback onMessage
    );
    // 发送消息到websocket
    NetworkCoroutine SendToWebsocket(
        HINTERNET hWebsocket,
        const std::vector<uint8_t>& data
    );

    namespace HttpsCorouineUtils
    {
        // HTTP操作包装
        struct HttpsAwaiter {
            HINTERNET handle;
            std::string result;
            bool completed = false;

            bool await_ready() const { return completed; }
            void await_suspend(std::coroutine_handle<> h) {
                WinHttpSetStatusCallback(handle, [](HINTERNET, DWORD_PTR dwContext, DWORD internetStatus, LPVOID, DWORD) {
                    auto* awaiter = reinterpret_cast<HttpsAwaiter*>(dwContext);
                    awaiter->completed = true;
                    std::coroutine_handle<>::from_address(awaiter).resume();
                    }, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
            }
            std::string await_resume() { return result; }
        };
        HttpsAwaiter SendRequestAsync(HINTERNET hRequest, const TCHAR* body = nullptr);
        HttpsAwaiter ReadResponseAsync(HINTERNET hRequest);
    }
};
