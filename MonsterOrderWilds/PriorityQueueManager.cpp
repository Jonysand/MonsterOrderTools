#include "framework.h"
#include "PriorityQueueManager.h"
#include "WriteLog.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

DEFINE_SINGLETON(PriorityQueueManager)

PriorityQueueManager::PriorityQueueManager()
    : saveDir_("MonsterOrderWilds_configs")
    , saveFileName_("OrderList.list")
    , lastSaveTime_(std::chrono::steady_clock::now())
{
}

bool PriorityQueueManager::Enqueue(QueueNodeData node)
{
    {
        std::lock_guard<Lock> guard(lock_);
        if (userIds_.count(node.userId))
        {
            return false;
        }
        queue_.push_back(node);
        userIds_.insert(node.userId);
        dirty_ = true;
    }
    NotifyQueueChanged();
    return true;
}

QueueNodeData PriorityQueueManager::Dequeue(int index)
{
    QueueNodeData node;
    {
        std::lock_guard<Lock> guard(lock_);
        if (index < 0 || index >= static_cast<int>(queue_.size()))
        {
            throw std::out_of_range("PriorityQueueManager: queue index out of range");
        }
        node = queue_[index];
        queue_.erase(queue_.begin() + index);
        userIds_.erase(node.userId);
        dirty_ = true;
    }
    NotifyQueueChanged();
    return node;
}

QueueNodeData PriorityQueueManager::Peek() const
{
    std::lock_guard<Lock> guard(lock_);
    if (queue_.empty())
    {
        throw std::runtime_error("PriorityQueueManager: queue is empty");
    }
    return queue_[0];
}

bool PriorityQueueManager::Contains(const std::string& userId) const
{
    std::lock_guard<Lock> guard(lock_);
    return userIds_.count(userId) > 0;
}

bool PriorityQueueManager::UpdateNodePriority(const std::string& userId)
{
    {
        std::lock_guard<Lock> guard(lock_);
        for (auto& node : queue_)
        {
            if (node.userId == userId)
            {
                node.priority = true;
                dirty_ = true;
                break;
            }
        }
    }
    // Re-sort since priority changed
    SortQueue();
    NotifyQueueChanged();
    return true;
}

void PriorityQueueManager::SortQueue()
{
    std::lock_guard<Lock> guard(lock_);
    std::sort(queue_.begin(), queue_.end());
}

void PriorityQueueManager::Clear()
{
    {
        std::lock_guard<Lock> guard(lock_);
        queue_.clear();
        userIds_.clear();
        dirty_ = true;
    }
    NotifyQueueChanged();
}

int PriorityQueueManager::GetCount() const
{
    std::lock_guard<Lock> guard(lock_);
    return static_cast<int>(queue_.size());
}

std::vector<QueueNodeData> PriorityQueueManager::GetAllNodes() const
{
    std::lock_guard<Lock> guard(lock_);
    return queue_;
}

bool PriorityQueueManager::LoadList(const std::string& configPath)
{
    try
    {
        std::string path = configPath;
        if (path.empty())
        {
            path = saveDir_ + "/" + saveFileName_;
        }

        if (!std::filesystem::exists(path))
            return false;

        std::ifstream file(path);
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // 跳过BOM
        if (content.size() >= 3 &&
            (unsigned char)content[0] == 0xEF &&
            (unsigned char)content[1] == 0xBB &&
            (unsigned char)content[2] == 0xBF)
        {
            content = content.substr(3);
        }

        json j = json::parse(content);

        std::lock_guard<Lock> guard(lock_);
        queue_.clear();
        userIds_.clear();

        for (const auto& item : j)
        {
            QueueNodeData node;
            if (item.contains("UserId")) node.userId = item["UserId"].get<std::string>();
            if (item.contains("TimeStamp")) node.timeStamp = item["TimeStamp"].get<long long>();
            if (item.contains("Priority")) node.priority = item["Priority"].get<bool>();
            if (item.contains("UserName")) node.userName = item["UserName"].get<std::string>();
            if (item.contains("MonsterName")) node.monsterName = item["MonsterName"].get<std::string>();
            if (item.contains("GuardLevel")) node.guardLevel = item["GuardLevel"].get<int>();
            if (item.contains("TemperedLevel")) node.temperedLevel = item["TemperedLevel"].get<int>();

            queue_.push_back(node);
            if (!node.userId.empty())
                userIds_.insert(node.userId);
        }

        return true;
    }
    catch (const json::parse_error& e)
    {
        LOG_ERROR(TEXT("[TEST] 7.9 BoundaryTest: LoadList JSON parse error at byte %d: %s"), (int)e.byte, e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("PriorityQueueManager: LoadList failed: %s"), e.what());
        return false;
    }
}

bool PriorityQueueManager::SaveList(const std::string& configPath)
{
    try
    {
        std::string dir = saveDir_;
        std::string path = configPath;
        if (path.empty())
        {
            if (!std::filesystem::exists(dir))
                std::filesystem::create_directories(dir);
            path = dir + "/" + saveFileName_;
        }

        json j = json::array();
        {
            std::lock_guard<Lock> guard(lock_);
            for (const auto& node : queue_)
            {
                json item;
                item["UserId"] = node.userId;
                item["TimeStamp"] = node.timeStamp;
                item["Priority"] = node.priority;
                item["UserName"] = node.userName;
                item["MonsterName"] = node.monsterName;
                item["GuardLevel"] = node.guardLevel;
                item["TemperedLevel"] = node.temperedLevel;
                j.push_back(item);
            }
        }

        std::string tempPath = path + ".tmp";
        std::ofstream tempFile(tempPath);
        if (!tempFile.is_open())
        {
            LOG_ERROR(TEXT("PriorityQueueManager: Cannot open temp file for writing: %s"), tempPath.c_str());
            return false;
        }

        tempFile << j.dump(4);
        tempFile.close();

        std::filesystem::rename(tempPath, path);

        dirty_ = false;
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(TEXT("PriorityQueueManager: SaveList failed: %s"), e.what());
        return false;
    }
}

void PriorityQueueManager::MarkDirty()
{
    dirty_ = true;
}

void PriorityQueueManager::Tick()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSaveTime_).count();

    if (dirty_ && elapsed >= SAVE_INTERVAL_MS)
    {
        SaveList();
        lastSaveTime_ = now;
    }
}

void PriorityQueueManager::AddQueueChangedListener(const QueueChangedHandler& handler)
{
    queueChangedListeners_.push_back(handler);
}

void PriorityQueueManager::NotifyQueueChanged()
{
    for (const auto& handler : queueChangedListeners_)
    {
        handler();
    }
}
