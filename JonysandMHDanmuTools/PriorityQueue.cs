using System;
using System.Collections.Generic;

namespace JonysandMHDanmuTools
{
    public class PriorityQueue
    {
        private SortedSet<PriorityQueueNode> _queue;

        public int Count => _queue.Count;

        public void Enqueue(long userId, long timeStamp, bool priority)
        {
            _queue.Add(new PriorityQueueNode(userId, timeStamp, priority));
        }

        public PriorityQueueNode Dequeue()
        {
            if (Count == 0)
            {
                throw new InvalidOperationException("queue is empty");
            }

            var node = _queue.Min;
            _queue.Remove(node);
            return node;
        }

        public PriorityQueueNode Peek()
        {
            if (Count == 0)
            {
                throw new InvalidOperationException("queue is empty");
            }

            return _queue.Min;
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
    }

    public class PriorityQueueNode : IComparable<PriorityQueueNode>
    {
        public long UserId;
        public long TimeStamp;
        public bool Priority;

        public PriorityQueueNode(long userId, long timeStamp, bool priority)
        {
            UserId = userId;
            TimeStamp = timeStamp;
            Priority = priority;
        }

        public int CompareTo(PriorityQueueNode other)
        {
            if (Priority && !other.Priority)
            {
                return -1;
            }

            if (Priority && other.Priority)
            {
                return TimeStamp < other.TimeStamp ? -1 : 1;
            }

            return 1;
        }
    }

}