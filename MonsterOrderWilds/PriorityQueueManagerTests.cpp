#include "framework.h"
#include "PriorityQueueManager.h"
#include "UnitTestLog.h"
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestEnqueue_Basic()
{
    // 获取新实例（单例，可能有之前测试的数据）
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();
    int before = mgr->GetCount();

    QueueNodeData node;
    node.userId = "test_user_1";
    node.userName = "测试用户";
    node.monsterName = "火龙";
    node.timeStamp = 1000;
    node.priority = false;
    node.guardLevel = 0;
    node.temperedLevel = 0;

    mgr->Enqueue(node);
    assert(mgr->GetCount() == before + 1);
    assert(mgr->Contains("test_user_1"));

    TestLog("[PASS] TestEnqueue_Basic");
}

void TestDequeue_Basic()
{
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();

    QueueNodeData node;
    node.userId = "test_user_dequeue";
    node.userName = "出队用户";
    node.monsterName = "火龙";
    node.timeStamp = 2000;
    node.priority = false;

    mgr->Enqueue(node);
    int count = mgr->GetCount();

    QueueNodeData dequeued = mgr->Dequeue(count - 1);
    assert(dequeued.userId == "test_user_dequeue");
    assert(!mgr->Contains("test_user_dequeue"));

    TestLog("[PASS] TestDequeue_Basic");
}

void TestContains_Basic()
{
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();

    QueueNodeData node;
    node.userId = "contains_test";
    node.userName = "包含测试";

    mgr->Enqueue(node);
    assert(mgr->Contains("contains_test"));
    assert(!mgr->Contains("nonexistent_user"));

    // 清理
    int idx = mgr->GetCount() - 1;
    while (idx >= 0 && mgr->GetCount() > 0)
    {
        auto n = mgr->Dequeue(idx);
        if (n.userId == "contains_test") break;
        idx--;
    }

    TestLog("[PASS] TestContains_Basic");
}

void TestSortQueue_PriorityFirst()
{
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();

    // 清空队列
    mgr->Clear();

    // 添加普通用户
    QueueNodeData normal;
    normal.userId = "normal_1";
    normal.userName = "普通用户";
    normal.timeStamp = 100;
    normal.priority = false;
    normal.guardLevel = 0;
    mgr->Enqueue(normal);

    // 添加舰长（优先）
    QueueNodeData vip;
    vip.userId = "vip_1";
    vip.userName = "舰长";
    vip.timeStamp = 200;
    vip.priority = true;
    vip.guardLevel = 3;
    mgr->Enqueue(vip);

    // 添加另一个普通用户
    QueueNodeData normal2;
    normal2.userId = "normal_2";
    normal2.userName = "普通用户2";
    normal2.timeStamp = 50;
    normal2.priority = false;
    normal2.guardLevel = 0;
    mgr->Enqueue(normal2);

    mgr->SortQueue();

    auto nodes = mgr->GetAllNodes();
    assert(nodes.size() == 3);
    // 舰长应该排在最前面（priority=true）
    assert(nodes[0].priority == true);
    assert(nodes[0].userId == "vip_1");

    TestLog("[PASS] TestSortQueue_PriorityFirst");
}

void TestGetAllNodes()
{
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();
    auto nodes = mgr->GetAllNodes();
    assert(nodes.size() == (size_t)mgr->GetCount());

    TestLog("[PASS] TestGetAllNodes");
}

void TestClear()
{
    PriorityQueueManager* mgr = PriorityQueueManager::Inst();
    mgr->Clear();
    assert(mgr->GetCount() == 0);

    TestLog("[PASS] TestClear");
}

// 运行所有测试
void RunAllPriorityQueueManagerTests()
{
    TestLog("=== PriorityQueueManager Tests ===");
    TestEnqueue_Basic();
    TestDequeue_Basic();
    TestContains_Basic();
    TestSortQueue_PriorityFirst();
    TestGetAllNodes();
    TestClear();
    TestLog("=== PriorityQueueManager Tests Done ===");
}

#endif // RUN_UNIT_TESTS