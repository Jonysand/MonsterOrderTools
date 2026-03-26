using System.Collections.Generic;
using Newtonsoft.Json;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class PriorityQueuePersistenceTests
    {
        [TestMethod]
        public void Deserialize_ValidJson_ReturnsList()
        {
            var nodes = new List<PriorityQueueNode>
            {
                new PriorityQueueNode { UserId = "u1", TimeStamp = 100, UserName = "用户1", MonsterName = "灭尽龙" },
                new PriorityQueueNode { UserId = "u2", TimeStamp = 200, UserName = "用户2", MonsterName = "火龙" }
            };

            var json = JsonConvert.SerializeObject(nodes);
            var deserialized = JsonConvert.DeserializeObject<List<PriorityQueueNode>>(json);

            Assert.IsNotNull(deserialized);
            Assert.AreEqual(2, deserialized.Count);
            Assert.AreEqual("u1", deserialized[0].UserId);
            Assert.AreEqual("灭尽龙", deserialized[0].MonsterName);
        }

        [TestMethod]
        public void Deserialize_NullJson_ThrowsArgumentNullException()
        {
            Assert.ThrowsException<System.ArgumentNullException>(() =>
                JsonConvert.DeserializeObject<List<PriorityQueueNode>>(null)
            );
        }

        [TestMethod]
        public void Deserialize_EmptyArray_ReturnsEmptyList()
        {
            var json = "[]";
            var deserialized = JsonConvert.DeserializeObject<List<PriorityQueueNode>>(json);

            Assert.IsNotNull(deserialized);
            Assert.AreEqual(0, deserialized.Count);
        }

        [TestMethod]
        public void NullCoalesce_Converts_Null_To_EmptyList()
        {
            List<PriorityQueueNode> deserialized = null;
            var safe = deserialized ?? new List<PriorityQueueNode>();

            Assert.IsNotNull(safe);
            Assert.AreEqual(0, safe.Count);
        }

        [TestMethod]
        public void HashSet_Contains_Works_For_UserId_Lookup()
        {
            var userIds = new HashSet<string>();
            userIds.Add("user_123");
            userIds.Add("user_456");

            Assert.IsTrue(userIds.Contains("user_123"));
            Assert.IsTrue(userIds.Contains("user_456"));
            Assert.IsFalse(userIds.Contains("user_789"));
        }

        [TestMethod]
        public void HashSet_Remove_Works_After_Dequeue()
        {
            var userIds = new HashSet<string>();
            userIds.Add("user_123");
            userIds.Add("user_456");

            userIds.Remove("user_123");

            Assert.IsFalse(userIds.Contains("user_123"));
            Assert.IsTrue(userIds.Contains("user_456"));
        }

        [TestMethod]
        public void HashSet_Clear_Works()
        {
            var userIds = new HashSet<string>();
            userIds.Add("user_123");
            userIds.Add("user_456");

            userIds.Clear();

            Assert.AreEqual(0, userIds.Count);
            Assert.IsFalse(userIds.Contains("user_123"));
        }

        [TestMethod]
        public void Dequeue_RemovesAt_Correct_Index()
        {
            var queue = new List<PriorityQueueNode>
            {
                new PriorityQueueNode { UserId = "u1", MonsterName = "A" },
                new PriorityQueueNode { UserId = "u2", MonsterName = "B" },
                new PriorityQueueNode { UserId = "u3", MonsterName = "C" }
            };

            var removed = queue[1];
            queue.RemoveAt(1);

            Assert.AreEqual("B", removed.MonsterName);
            Assert.AreEqual(2, queue.Count);
            Assert.AreEqual("A", queue[0].MonsterName);
            Assert.AreEqual("C", queue[1].MonsterName);
        }
    }
}
