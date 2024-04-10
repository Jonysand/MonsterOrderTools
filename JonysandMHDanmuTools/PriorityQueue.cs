using System;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace JonysandMHDanmuTools
{
    public class PriorityQueue
    {
        private readonly List<PriorityQueueNode> _queue = new List<PriorityQueueNode>();

        public List<PriorityQueueNode> Queue { get { return _queue; } }

        public int Count => _queue.Count;

        public void Enqueue(long userId, long timeStamp, bool priority, string userName, string monsterName)
        {
            _queue.Add(new PriorityQueueNode(userId, timeStamp, priority, userName, monsterName));
            if (priority)
            {
                SortQueue();
            }
        }

        public PriorityQueueNode Dequeue()
        {
            if (Count == 0)
            {
                throw new InvalidOperationException("queue is empty");
            }

            var node = _queue[0];
            _queue.Remove(node);
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

        public bool Contains(long userId)
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
    }

    public class PriorityQueueNode : IComparable<PriorityQueueNode>
    {
        public long UserId;
        public long TimeStamp;
        public bool Priority; //todo 可能要从bool改为int,如果要排总督、提督、舰长。同时改一下CompareTo
        public string UserName;
        public string MonsterName;

        public PriorityQueueNode(long userId, long timeStamp, bool priority, string userName, string monsterName)
        {
            UserId = userId;
            TimeStamp = timeStamp;
            Priority = priority;
            UserName = userName;
            MonsterName = monsterName;
        }

        public int CompareTo(PriorityQueueNode other)
        {
            if (Priority != other.Priority)
            {
                return Priority ? -1 : 1;
            }
            
            return TimeStamp < other.TimeStamp ? -1 : 1;
        }
    }

}