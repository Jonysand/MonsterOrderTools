using System;
using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;

namespace MonsterOrderWindows
{
    public class PriorityQueue
    {
        private List<PriorityQueueNode> _queue = new List<PriorityQueueNode>();
        private HashSet<string> _userIds = new HashSet<string>();

        public List<PriorityQueueNode> Queue { get { return _queue; } }

        public int Count => _queue.Count;

        private string _saveDir = null;
        private string _saveFileName = null;

        private bool _dirty = false;
        private System.Windows.Threading.DispatcherTimer _saveTimer;

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

            _saveTimer = new System.Windows.Threading.DispatcherTimer();
            _saveTimer.Interval = TimeSpan.FromMilliseconds(500);
            _saveTimer.Tick += (s, e) =>
            {
                if (_dirty)
                {
                    SaveList();
                    _dirty = false;
                }
            };
            _saveTimer.Start();
        }

        public void Enqueue(PriorityQueueNode node)
        {
            _queue.Add(node);
            _userIds.Add(node.UserId);
            _dirty = true;
        }

        public PriorityQueueNode Dequeue(int index)
        {
            if (index < 0 || index >= Count)
            {
                throw new InvalidOperationException("queue size error");
            }
            PriorityQueueNode node = _queue[index];
            _queue.RemoveAt(index);
            _userIds.Remove(node.UserId);
            _dirty = true;
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
            return _userIds.Contains(userId);
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
            var deserialized = JsonConvert.DeserializeObject<List<PriorityQueueNode>>(json);
            _queue = deserialized ?? new List<PriorityQueueNode>();
            _userIds.Clear();
            foreach (var node in _queue)
            {
                if (!string.IsNullOrEmpty(node.UserId))
                    _userIds.Add(node.UserId);
            }
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
            _userIds.Clear();
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