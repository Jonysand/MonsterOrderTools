using System;
using System.Collections.Generic;
using System.IO;

namespace MonsterOrderWindows
{
    public class PriorityQueue
    {
        private List<PriorityQueueNode> _queue = new List<PriorityQueueNode>();
        private HashSet<string> _userIds = new HashSet<string>();

        public List<PriorityQueueNode> Queue { get { return _queue; } }

        public int Count => _queue.Count;

        private static PriorityQueue _Inst = null;

        public static PriorityQueue GetInst()
        {
            if (_Inst != null)
                return _Inst;
            _Inst = new PriorityQueue();
            return _Inst;
        }

        public PriorityQueue()
        {
            RefreshFromNative();
        }

        private void RefreshFromNative()
        {
            _queue.Clear();
            _userIds.Clear();
            try
            {
                var nodes = new List<PriorityQueueNode>();
                NativeImports.OnQueueNodeCallback callback = (userId, timeStamp, priority, userName, monsterName, guardLevel, temperedLevel, userData) =>
                {
                    var node = new PriorityQueueNode
                    {
                        UserId = userId,
                        TimeStamp = timeStamp,
                        Priority = priority,
                        UserName = userName,
                        MonsterName = monsterName,
                        GuardLevel = guardLevel,
                        TemperedLevel = temperedLevel
                    };
                    nodes.Add(node);
                };
                NativeImports.PriorityQueue_GetAllNodes(callback, IntPtr.Zero);
                foreach (var node in nodes)
                {
                    _queue.Add(node);
                    _userIds.Add(node.UserId);
                }
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_GetAllNodes failed: " + e.Message);
            }
        }

        public void Enqueue(PriorityQueueNode node)
        {
            try
            {
                NativeImports.PriorityQueue_Enqueue(
                    node.UserId,
                    node.TimeStamp,
                    node.Priority,
                    node.UserName,
                    node.MonsterName,
                    node.GuardLevel,
                    node.TemperedLevel);
                _queue.Add(node);
                _userIds.Add(node.UserId);
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_Enqueue failed: " + e.Message);
            }
        }

        public PriorityQueueNode Dequeue(int index)
        {
            if (index < 0 || index >= Count)
            {
                throw new InvalidOperationException("queue size error");
            }
            PriorityQueueNode node = _queue[index];
            try
            {
                NativeImports.PriorityQueue_DequeueByIndex(index);
                _queue.RemoveAt(index);
                _userIds.Remove(node.UserId);
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_DequeueByIndex failed: " + e.Message);
            }
            return node;
        }

        public PriorityQueueNode Peek()
        {
            if (Count == 0)
            {
                throw new InvalidOperationException("queue is empty");
            }
            return _queue[0];
        }

        public bool Contains(string userId)
        {
            try
            {
                return NativeImports.PriorityQueue_Contains(userId);
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_Contains failed: " + e.Message);
                return _userIds.Contains(userId);
            }
        }

        public void SortQueue()
        {
            try
            {
                NativeImports.PriorityQueue_SortQueue();
                RefreshFromNative();
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_SortQueue failed: " + e.Message);
            }
        }

        public bool LoadList()
        {
            try
            {
                bool result = NativeImports.PriorityQueue_LoadList();
                if (result)
                {
                    RefreshFromNative();
                }
                return result;
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_LoadList failed: " + e.Message);
                return false;
            }
        }

        public void SaveList()
        {
            try
            {
                NativeImports.PriorityQueue_SaveList();
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_SaveList failed: " + e.Message);
            }
        }

        public void Clear()
        {
            try
            {
                NativeImports.PriorityQueue_Clear();
                _queue.Clear();
                _userIds.Clear();
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:PriorityQueue_Clear failed: " + e.Message);
            }
        }
    }

    public class PriorityQueueNode : IComparable<PriorityQueueNode>
    {
        public string UserId;
        public long TimeStamp;
        public bool Priority; //todo 可能要从bool改为int,如果要排总督、提督、舰长。同时改一下CompareTo
        public string UserName;
        public string MonsterName;
        public int GuardLevel;
        public int TemperedLevel = 0; // 0 - 普通，1 - 历战，2 - 历战王

        public PriorityQueueNode()
        {
        }
        public PriorityQueueNode(string userId, long timeStamp, bool priority, string userName, string monsterName, int guardLevel)
        {
            UserId = userId;
            TimeStamp = timeStamp;
            Priority = priority;
            UserName = userName;
            MonsterName = monsterName;
            GuardLevel = guardLevel;
        }

        public int CompareTo(PriorityQueueNode other)
        {
            if (Priority != other.Priority)
                return Priority ? -1 : 1;
            if (Priority && other.Priority)
            {
                if (GuardLevel != other.GuardLevel)
                    return GuardLevel > other.GuardLevel ? -1 : 1;
                return TimeStamp < other.TimeStamp ? -1 : 1;
            }
            return TimeStamp < other.TimeStamp ? -1 : 1;
        }
    }

}