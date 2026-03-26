using System;
using Newtonsoft.Json;

namespace JonysandMHDanmuTools.Tests
{
    /// <summary>
    /// 从主项目复制的 PriorityQueueNode（无 WPF 依赖）
    /// 用于独立测试排序逻辑
    /// </summary>
    [JsonObject(MemberSerialization = MemberSerialization.OptIn)]
    public class PriorityQueueNode : IComparable<PriorityQueueNode>
    {
        [JsonProperty]
        public string UserId;
        [JsonProperty]
        public long TimeStamp;
        [JsonProperty]
        public bool Priority;
        [JsonProperty]
        public string UserName;
        [JsonProperty]
        public string MonsterName;
        [JsonProperty]
        public int GuardLevel;
        [JsonProperty]
        public int TemperedLevel = 0;

        public PriorityQueueNode() { }

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
