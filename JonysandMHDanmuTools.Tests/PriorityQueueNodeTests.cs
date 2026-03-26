using System;
using System.Text.RegularExpressions;
using Newtonsoft.Json;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class PriorityQueueNodeTests
    {
        [TestMethod]
        public void CompareTo_PriorityTrue_Vs_PriorityFalse_PriorityTrueWins()
        {
            var a = new PriorityQueueNode { Priority = true, TimeStamp = 100 };
            var b = new PriorityQueueNode { Priority = false, TimeStamp = 50 };

            Assert.IsTrue(a.CompareTo(b) < 0, "Priority=true 应排在 Priority=false 前面");
        }

        [TestMethod]
        public void CompareTo_PriorityFalse_Vs_PriorityTrue_PriorityTrueWins()
        {
            var a = new PriorityQueueNode { Priority = false, TimeStamp = 50 };
            var b = new PriorityQueueNode { Priority = true, TimeStamp = 100 };

            Assert.IsTrue(a.CompareTo(b) > 0, "Priority=false 应排在 Priority=true 后面");
        }

        [TestMethod]
        public void CompareTo_BothPriority_HigherGuardLevel_Wins()
        {
            var a = new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 100 };
            var b = new PriorityQueueNode { Priority = true, GuardLevel = 1, TimeStamp = 50 };

            Assert.IsTrue(a.CompareTo(b) < 0, "GuardLevel 高的应排在前面");
        }

        [TestMethod]
        public void CompareTo_BothPriority_LowerGuardLevel_Loses()
        {
            var a = new PriorityQueueNode { Priority = true, GuardLevel = 1, TimeStamp = 50 };
            var b = new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 100 };

            Assert.IsTrue(a.CompareTo(b) > 0, "GuardLevel 低的应排在后面");
        }

        [TestMethod]
        public void CompareTo_BothPriority_SameGuardLevel_EarlierTimeStamp_Wins()
        {
            var a = new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 50 };
            var b = new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 100 };

            Assert.IsTrue(a.CompareTo(b) < 0, "同 GuardLevel 时，时间戳早的应排在前面");
        }

        [TestMethod]
        public void CompareTo_BothNotPriority_EarlierTimeStamp_Wins()
        {
            var a = new PriorityQueueNode { Priority = false, TimeStamp = 50 };
            var b = new PriorityQueueNode { Priority = false, TimeStamp = 100 };

            Assert.IsTrue(a.CompareTo(b) < 0, "都非优先时，时间戳早的应排在前面");
        }

        [TestMethod]
        public void CompareTo_BothNotPriority_LaterTimeStamp_Loses()
        {
            var a = new PriorityQueueNode { Priority = false, TimeStamp = 200 };
            var b = new PriorityQueueNode { Priority = false, TimeStamp = 100 };

            Assert.IsTrue(a.CompareTo(b) > 0, "都非优先时，时间戳晚的应排在后面");
        }

        [TestMethod]
        public void CompareTo_Sorting_Produces_Correct_Order()
        {
            var nodes = new[]
            {
                new PriorityQueueNode { Priority = false, TimeStamp = 300, UserName = "普通晚" },
                new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 200, UserName = "舰长" },
                new PriorityQueueNode { Priority = true, GuardLevel = 1, TimeStamp = 100, UserName = "总督" },
                new PriorityQueueNode { Priority = false, TimeStamp = 100, UserName = "普通早" },
                new PriorityQueueNode { Priority = true, GuardLevel = 3, TimeStamp = 50, UserName = "舰长早" },
            };

            Array.Sort(nodes);

            Assert.AreEqual("舰长早", nodes[0].UserName);
            Assert.AreEqual("舰长", nodes[1].UserName);
            Assert.AreEqual("总督", nodes[2].UserName);
            Assert.AreEqual("普通早", nodes[3].UserName);
            Assert.AreEqual("普通晚", nodes[4].UserName);
        }

        [TestMethod]
        public void Serialize_Deserialize_Preserves_Data()
        {
            var node = new PriorityQueueNode
            {
                UserId = "test_id",
                TimeStamp = 1234567890,
                Priority = true,
                UserName = "测试用户[舰长]",
                MonsterName = "灭尽龙",
                GuardLevel = 3,
                TemperedLevel = 1
            };

            var json = JsonConvert.SerializeObject(node);
            var deserialized = JsonConvert.DeserializeObject<PriorityQueueNode>(json);

            Assert.AreEqual(node.UserId, deserialized.UserId);
            Assert.AreEqual(node.TimeStamp, deserialized.TimeStamp);
            Assert.AreEqual(node.Priority, deserialized.Priority);
            Assert.AreEqual(node.UserName, deserialized.UserName);
            Assert.AreEqual(node.MonsterName, deserialized.MonsterName);
            Assert.AreEqual(node.GuardLevel, deserialized.GuardLevel);
            Assert.AreEqual(node.TemperedLevel, deserialized.TemperedLevel);
        }
    }
}
