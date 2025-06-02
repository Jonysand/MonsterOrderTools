using System;
using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;

namespace MonsterOrderWindows
{
    public class PriorityQueue
    {
        private List<PriorityQueueNode> _queue = new List<PriorityQueueNode>();

        public List<PriorityQueueNode> Queue { get { return _queue; } }

        public int Count => _queue.Count;

        private string _saveDir = null;
        private string _saveFileName = null;

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
            _saveDir = Path.Combine(Environment.CurrentDirectory, @"MonsterOrderWilds_configs");
            _saveFileName = "OrderList.list";
            LoadList();
        }

        public void Enqueue(PriorityQueueNode node)
        {
            _queue.Add(node);
            SaveList();
        }

        public PriorityQueueNode Dequeue(int index)
        {
            if (index < 0 || index >= Count)
            {
                throw new InvalidOperationException("queue size error");
            }
            PriorityQueueNode node = _queue[index];
            _queue.Remove(node);
            SaveList();
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
            foreach (var node in _queue)
            {
                if (node.UserId == userId)
                {
                    return true;
                }
            }
            return false;
        }

        public void SortQueue()
        {
            _queue.Sort((a, b) => a.CompareTo(b));
        }

        public bool LoadList()
        {
            if (_saveDir == null || _saveFileName == null)
                return false;

            string orderListPath = Path.Combine(_saveDir, _saveFileName);
            if (!File.Exists(orderListPath))
                return false;
            string json = File.ReadAllText(orderListPath);
            _queue = JsonConvert.DeserializeObject<List<PriorityQueueNode>>(json);
            return true;
        }

        public void SaveList()
        {
            if (_saveDir == null || _saveFileName == null)
                return;

            if (!Directory.Exists(_saveDir))
            {
                Directory.CreateDirectory(_saveDir);
            }
            string configPath = Path.Combine(_saveDir, _saveFileName);
            File.WriteAllText(configPath, JsonConvert.SerializeObject(Queue));
        }

        public void Clear()
        {
            Queue.Clear();
            SaveList();
        }
    }

    [JsonObject(MemberSerialization = MemberSerialization.OptIn)]
    public class PriorityQueueNode : IComparable<PriorityQueueNode>
    {
        [JsonProperty]
        public string UserId;
        [JsonProperty]
        public long TimeStamp;
        [JsonProperty]
        public bool Priority; //todo 可能要从bool改为int,如果要排总督、提督、舰长。同时改一下CompareTo
        [JsonProperty]
        public string UserName;
        [JsonProperty]
        public string MonsterName;
        [JsonProperty]
        public int GuardLevel;
        [JsonProperty]
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
            // 如果一个优先另一个不优先，直接排序
            if (Priority != other.Priority)
                return Priority ? -1:1;
            // 如果都优先，按舰长等级
            else if (Priority && other.Priority)
            {
                if (GuardLevel > other.GuardLevel)
                    return -1;
                return TimeStamp < other.TimeStamp ? -1 : 1;
            }
            else
                return TimeStamp < other.TimeStamp ? -1 : 1;
        }
    }

}