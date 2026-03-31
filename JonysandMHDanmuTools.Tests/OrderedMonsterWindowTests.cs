using System;
using System.Collections.ObjectModel;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class OrderedMonsterWindowTests
    {
        [TestMethod]
        public void TestObservableCollectionMove_ValidIndices_MovesItem()
        {
            var collection = new ObservableCollection<TestOrderInfo>();
            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            collection.Add(new TestOrderInfo("user2", "火龙"));
            collection.Add(new TestOrderInfo("user3", "雷狼龙"));

            Assert.AreEqual("user1", collection[0].AudienceName);
            Assert.AreEqual("user2", collection[1].AudienceName);
            Assert.AreEqual("user3", collection[2].AudienceName);

            collection.Move(0, 2);

            Assert.AreEqual("user2", collection[0].AudienceName);
            Assert.AreEqual("user3", collection[1].AudienceName);
            Assert.AreEqual("user1", collection[2].AudienceName);
        }

        [TestMethod]
        public void TestObservableCollectionMove_SameIndex_NoChange()
        {
            var collection = new ObservableCollection<TestOrderInfo>();
            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            collection.Add(new TestOrderInfo("user2", "火龙"));

            collection.Move(0, 0);

            Assert.AreEqual("user1", collection[0].AudienceName);
            Assert.AreEqual("user2", collection[1].AudienceName);
        }

        [TestMethod]
        public void TestObservableCollectionMove_AdjacentIndices_MovesCorrectly()
        {
            var collection = new ObservableCollection<TestOrderInfo>();
            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            collection.Add(new TestOrderInfo("user2", "火龙"));
            collection.Add(new TestOrderInfo("user3", "雷狼龙"));

            collection.Move(0, 1);

            Assert.AreEqual("user2", collection[0].AudienceName);
            Assert.AreEqual("user1", collection[1].AudienceName);
            Assert.AreEqual("user3", collection[2].AudienceName);
        }

        [TestMethod]
        public void TestThrottleLogic_Within100Ms_IgnoresSecondCall()
        {
            var lastRefreshTime = DateTime.MinValue;
            const int THROTTLE_MS = 100;
            int callCount = 0;

            Action tryRefresh = () =>
            {
                var now = DateTime.Now;
                if ((now - lastRefreshTime).TotalMilliseconds < THROTTLE_MS)
                    return;
                lastRefreshTime = now;
                callCount++;
            };

            tryRefresh();
            Assert.AreEqual(1, callCount);

            Thread.Sleep(50);
            tryRefresh();
            Assert.AreEqual(1, callCount, "Within 100ms - second call should be ignored");

            Thread.Sleep(60);
            tryRefresh();
            Assert.AreEqual(2, callCount, "After 100ms+ - should allow new call");
        }

        [TestMethod]
        public void TestThrottleLogic_After100Ms_Executes()
        {
            var lastRefreshTime = DateTime.MinValue;
            const int THROTTLE_MS = 100;
            int callCount = 0;

            Action tryRefresh = () =>
            {
                var now = DateTime.Now;
                if ((now - lastRefreshTime).TotalMilliseconds < THROTTLE_MS)
                    return;
                lastRefreshTime = now;
                callCount++;
            };

            tryRefresh();
            Assert.AreEqual(1, callCount);

            Thread.Sleep(101);
            tryRefresh();
            Assert.AreEqual(2, callCount, "After 100ms delay - should execute");
        }

        [TestMethod]
        public void TestThrottleLogic_MultipleRapidCalls_OnlyFirstExecutes()
        {
            var lastRefreshTime = DateTime.MinValue;
            const int THROTTLE_MS = 100;
            int callCount = 0;

            Action tryRefresh = () =>
            {
                var now = DateTime.Now;
                if ((now - lastRefreshTime).TotalMilliseconds < THROTTLE_MS)
                    return;
                lastRefreshTime = now;
                callCount++;
            };

            for (int i = 0; i < 10; i++)
            {
                tryRefresh();
            }

            Assert.AreEqual(1, callCount, "Multiple rapid calls within 100ms - only first should execute");
        }

        [TestMethod]
        public void TestDispatcherInvokeAsync_CallsAction()
        {
            int counter = 0;
            var tcs = new TaskCompletionSource<bool>();

            var mockDispatcher = new MockDispatcher();
            mockDispatcher.InvokeAsyncCallback = (action) =>
            {
                action();
                tcs.TrySetResult(true);
            };

            mockDispatcher.InvokeAsync(() => { counter++; });

            tcs.Task.Wait(1000);

            Assert.AreEqual(1, counter, "Action should be called once");
        }

        [TestMethod]
        public void TestDispatcherInvokeAsync_ReturnsTask()
        {
            var mockDispatcher = new MockDispatcher();
            var task = mockDispatcher.InvokeAsync(() => { Thread.Sleep(10); });

            Assert.IsNotNull(task);
            Assert.IsInstanceOfType(task, typeof(Task));
        }

        [TestMethod]
        public void TestObservableCollection_NotifyCollectionChanged_OnMove()
        {
            var collection = new ObservableCollection<TestOrderInfo>();
            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            collection.Add(new TestOrderInfo("user2", "火龙"));

            int changeCount = 0;
            collection.CollectionChanged += (s, e) =>
            {
                if (e.Action == System.Collections.Specialized.NotifyCollectionChangedAction.Move)
                    changeCount++;
            };

            collection.Move(0, 1);

            Assert.AreEqual(1, changeCount, "Move should trigger NotifyCollectionChanged with Move action");
        }

        [TestMethod]
        public void TestObservableCollection_Clear_RemovesAllItems()
        {
            var collection = new ObservableCollection<TestOrderInfo>();
            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            collection.Add(new TestOrderInfo("user2", "火龙"));

            Assert.AreEqual(2, collection.Count);

            collection.Clear();

            Assert.AreEqual(0, collection.Count);
        }

        [TestMethod]
        public void TestObservableCollection_Add_IncreasesCount()
        {
            var collection = new ObservableCollection<TestOrderInfo>();

            Assert.AreEqual(0, collection.Count);

            collection.Add(new TestOrderInfo("user1", "灭尽龙"));
            Assert.AreEqual(1, collection.Count);

            collection.Add(new TestOrderInfo("user2", "火龙"));
            Assert.AreEqual(2, collection.Count);
        }

        [TestMethod]
        public void TestObservableCollection_Remove_DecreasesCount()
        {
            var item1 = new TestOrderInfo("user1", "灭尽龙");
            var item2 = new TestOrderInfo("user2", "火龙");

            var collection = new ObservableCollection<TestOrderInfo> { item1, item2 };
            Assert.AreEqual(2, collection.Count);

            collection.Remove(item1);

            Assert.AreEqual(1, collection.Count);
            Assert.AreEqual("user2", collection[0].AudienceName);
        }

        private class MockDispatcher
        {
            public Action<Action> InvokeAsyncCallback { get; set; }

            public Task InvokeAsync(Action callback)
            {
                InvokeAsyncCallback?.Invoke(callback);
                return Task.CompletedTask;
            }

            public Task InvokeAsync(Action callback, CancellationToken cancellationToken)
            {
                return InvokeAsync(callback);
            }

            public Task<T> InvokeAsync<T>(Func<T> callback)
            {
                var result = callback();
                return Task.FromResult(result);
            }

            public Task<T> InvokeAsync<T>(Func<T> callback, CancellationToken cancellationToken)
            {
                return InvokeAsync(callback);
            }
        }

        private class TestOrderInfo
        {
            public string AudienceName { get; set; }
            public string MonsterName { get; set; }
            public int GuardLevel { get; set; }
            public int TemperedLevel { get; set; }

            public TestOrderInfo() { }
            public TestOrderInfo(string audienceName, string monsterName)
            {
                AudienceName = audienceName;
                MonsterName = monsterName;
            }
        }
    }
}