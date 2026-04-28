#pragma once
#include "framework.h"
#include <vector>
#include <string>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <mutex>

// 队列节点数据结构（纯数据）
struct QueueNodeData
{
    std::string userId;
    long long timeStamp = 0;
    bool priority = false;
    std::string userName;
    std::string monsterName;
    int guardLevel = 0;
    int temperedLevel = 0;  // 0-普通，1-历战，2-历战王

    // 比较函数（与C#版本一致）
    int CompareTo(const QueueNodeData& other) const
    {
        if (priority != other.priority)
            return priority ? -1 : 1;
        if (priority && other.priority)
        {
            if (guardLevel != other.guardLevel)
                return guardLevel < other.guardLevel ? -1 : 1;
            return timeStamp < other.timeStamp ? -1 : 1;
        }
        return timeStamp < other.timeStamp ? -1 : 1;
    }

    bool operator<(const QueueNodeData& other) const
    {
        return CompareTo(other) < 0;
    }
};

// 优先级队列管理器 - 负责队列数据结构、排序、持久化
class PriorityQueueManager
{
    DECLARE_SINGLETON(PriorityQueueManager)

public:
    // 入队（返回false表示用户已在队列中）
    bool Enqueue(QueueNodeData node);
    // 出队（按索引）
    QueueNodeData Dequeue(int index);
    // 查看队首
    QueueNodeData Peek() const;
    // 检查用户是否在队列中
    bool Contains(const std::string& userId) const;
    // 更新用户的优先状态
    bool UpdateNodePriority(const std::string& userId);
    // 排序
    void SortQueue();
    // 清空队列
    void Clear();
    // 获取队列大小
    int GetCount() const;

    // 批量获取队列数据（用于C#代理类一次获取）
    std::vector<QueueNodeData> GetAllNodes() const;

    // 持久化
    bool LoadList(const std::string& configPath = "");
    bool SaveList(const std::string& configPath = "");
    void MarkDirty();

    // 定时保存Tick（在主线程调用）
    void Tick();

    // 队列变更事件
    using QueueChangedHandler = std::function<void()>;
    void AddQueueChangedListener(const QueueChangedHandler& handler);

private:
    PriorityQueueManager();
    ~PriorityQueueManager() = default;

    std::vector<QueueNodeData> queue_;
    std::unordered_set<std::string> userIds_;
    std::atomic<bool> dirty_{false};
    std::string saveDir_;
    std::string saveFileName_;

    // 定时保存
    std::chrono::steady_clock::time_point lastSaveTime_;
    static constexpr int SAVE_INTERVAL_MS = 500;

    // 线程安全
    mutable Lock lock_;

    // 队列变更监听器
    std::vector<QueueChangedHandler> queueChangedListeners_;
    void NotifyQueueChanged();
};
