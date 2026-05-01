#pragma once
#include "framework.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

class ProfileManager;

enum class WriteTaskType {
    LIKE,
    CHECKIN,
    RETROACTIVE_CHECKIN,
};

struct WriteTask {
    WriteTaskType type;
    std::string uid;
    std::string username;
    std::function<bool(ProfileManager*)> execute;
};

class WriteQueue {
    DECLARE_SINGLETON(WriteQueue)

public:
    WriteQueue();
    ~WriteQueue();
    void Enqueue(WriteTask task);
    void Flush();

private:
    void ProcessLoop();
    void FlushRemaining();

    std::queue<WriteTask> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> running_{true};
    bool workerStarted_ = false;

    static constexpr size_t MAX_QUEUE_SIZE = 10000;
    static constexpr int FLUSH_TIMEOUT_MS = 100;
    static constexpr int MAX_RETRIES = 3;
    static constexpr int RETRY_DELAY_MS = 50;
};
