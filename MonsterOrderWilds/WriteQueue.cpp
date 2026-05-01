#include "framework.h"
#include "WriteQueue.h"
#include "ProfileManager.h"
#include "WriteLog.h"

DEFINE_SINGLETON(WriteQueue)

WriteQueue::WriteQueue() {
    worker_ = std::thread(&WriteQueue::ProcessLoop, this);
    workerStarted_ = true;
}

WriteQueue::~WriteQueue() {
    running_ = false;
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    FlushRemaining();
}

void WriteQueue::Enqueue(WriteTask task) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return queue_.size() < MAX_QUEUE_SIZE || !running_;
        });
        if (!running_) {
            LOG_WARNING(TEXT("WriteQueue: Enqueue after Flush, task dropped uid=%hs type=%d"),
                task.uid.c_str(), static_cast<int>(task.type));
            return;
        }
        queue_.push(std::move(task));
    }
    cv_.notify_one();
}

void WriteQueue::Flush() {
    running_ = false;
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    FlushRemaining();
}

void WriteQueue::ProcessLoop() {
    while (running_) {
        std::vector<WriteTask> batch;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(FLUSH_TIMEOUT_MS), [this]() {
                return !queue_.empty() || !running_;
            });
            while (!queue_.empty() && batch.size() < 50) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop();
            }
        }
        for (auto& task : batch) {
            bool success = false;
            for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
                if (task.execute(ProfileManager::Inst())) {
                    success = true;
                    break;
                }
                LOG_WARNING(TEXT("WriteQueue: Task failed (attempt %d/%d), uid=%hs type=%d"),
                    attempt + 1, MAX_RETRIES, task.uid.c_str(), static_cast<int>(task.type));
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
            }
            if (success) {
                LOG_INFO(TEXT("WriteQueue: Task succeeded, uid=%hs username=%hs type=%d"),
                    task.uid.c_str(), task.username.c_str(), static_cast<int>(task.type));
            } else {
                LOG_ERROR(TEXT("WriteQueue: Task failed after %d retries, uid=%hs username=%hs type=%d"),
                    MAX_RETRIES, task.uid.c_str(), task.username.c_str(), static_cast<int>(task.type));
            }
        }
    }
}

void WriteQueue::FlushRemaining() {
    size_t dropped = 0;
    while (!queue_.empty()) {
        auto& task = queue_.front();
        bool success = false;
        for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
            if (task.execute(ProfileManager::Inst())) {
                success = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
        }
        if (!success) {
            ++dropped;
            LOG_ERROR(TEXT("WriteQueue: Flush dropped task, uid=%hs type=%d"),
                task.uid.c_str(), static_cast<int>(task.type));
        }
        queue_.pop();
    }
    if (dropped > 0) {
        LOG_ERROR(TEXT("WriteQueue: Flush completed with %zu dropped tasks"), dropped);
    }
}
