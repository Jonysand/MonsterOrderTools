using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class MonsterIconLoaderTests
    {
        [TestMethod]
        public void Test_LoadIcon_InvalidPath_ReturnsNull()
        {
            var result = MonsterOrderWindows.MonsterIconLoader.LoadIcon("");
            Assert.IsNull(result);
        }

        [TestMethod]
        public void Test_LoadIcon_NonExistentZip_ReturnsNull()
        {
            MonsterOrderWindows.MonsterIconLoader.Initialize("C:\\NonExistentPath");
            var result = MonsterOrderWindows.MonsterIconLoader.LoadIcon("test.png");
            Assert.IsNull(result);
        }
    }
}
