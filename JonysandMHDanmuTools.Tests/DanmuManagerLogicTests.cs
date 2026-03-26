using System;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class DanmuManagerLogicTests
    {
        private readonly Regex[] _orderPatterns;
        private readonly Regex[] _priorityPatterns;
        private static readonly Regex RegexTemperedKing = new Regex(@"^历战王", RegexOptions.Compiled);
        private static readonly Regex RegexTempered = new Regex(@"^历战", RegexOptions.Compiled);
        private static readonly Regex RegexKing = new Regex(@"^王", RegexOptions.Compiled);

        public DanmuManagerLogicTests()
        {
            _orderPatterns = new Regex[] {
                new Regex(@"^点怪", RegexOptions.Compiled),
                new Regex(@"^点个", RegexOptions.Compiled),
                new Regex(@"^点只", RegexOptions.Compiled),
                new Regex(@"^點怪", RegexOptions.Compiled),
                new Regex(@"^點個", RegexOptions.Compiled),
                new Regex(@"^點隻", RegexOptions.Compiled)
            };
            _priorityPatterns = new Regex[] {
                new Regex(@"优先", RegexOptions.Compiled),
                new Regex(@"插队", RegexOptions.Compiled),
                new Regex(@"優先", RegexOptions.Compiled),
                new Regex(@"插隊", RegexOptions.Compiled)
            };
        }

        private string NormalizeString(string data)
        {
            return data.Replace(" ", "").Replace(",", "").Replace("，", "");
        }

        private Tuple<string, int> NormalizeMonsterName(string monster_name)
        {
            int temperedLevel;
            if (RegexTemperedKing.Match(monster_name).Success)
            {
                temperedLevel = 2;
                monster_name = monster_name.Substring(3);
            }
            else if (RegexTempered.Match(monster_name).Success)
            {
                temperedLevel = 1;
                monster_name = monster_name.Substring(2);
            }
            else if (RegexKing.Match(monster_name).Success)
            {
                temperedLevel = 2;
                monster_name = monster_name.Substring(1);
            }
            else
                temperedLevel = 0;
            return new Tuple<string, int>(monster_name, temperedLevel);
        }

        private bool IsOrderMessage(string text, out string remainder)
        {
            remainder = null;
            foreach (var pattern in _orderPatterns)
            {
                var match = pattern.Match(text);
                if (match.Success)
                {
                    remainder = text.Substring(match.Index + 2);
                    return true;
                }
            }
            return false;
        }

        private bool HasPriorityKeyword(string text)
        {
            foreach (var pattern in _priorityPatterns)
            {
                if (pattern.Match(text).Success)
                    return true;
            }
            return false;
        }

        [DataTestMethod]
        [DataRow("点怪灭尽龙", true, "灭尽龙")]
        [DataRow("点个火龙", true, "火龙")]
        [DataRow("点只雷狼龙", true, "雷狼龙")]
        [DataRow("點怪轟龍", true, "轟龍")]
        [DataRow("點個迅龍", true, "迅龍")]
        [DataRow("點隻角龍", true, "角龍")]
        [DataRow("随便说点啥", false, null)]
        [DataRow("灭尽龙", false, null)]
        public void OrderPattern_Matches_Correctly(string input, bool expectedMatch, string expectedRemainder)
        {
            var normalized = NormalizeString(input);
            var matched = IsOrderMessage(normalized, out var remainder);
            Assert.AreEqual(expectedMatch, matched);
            if (expectedMatch)
                Assert.AreEqual(expectedRemainder, remainder);
        }

        [DataTestMethod]
        [DataRow("点怪灭尽龙优先", true)]
        [DataRow("点怪灭尽龙插队", true)]
        [DataRow("点怪優先灭尽龙", true)]
        [DataRow("點怪插隊灭尽龙", true)]
        [DataRow("点怪灭尽龙", false)]
        [DataRow("随便说点啥", false)]
        public void PriorityPattern_Matches_Correctly(string input, bool expectedMatch)
        {
            var normalized = NormalizeString(input);
            Assert.AreEqual(expectedMatch, HasPriorityKeyword(normalized));
        }

        [DataTestMethod]
        [DataRow("历战王灭尽龙", "灭尽龙", 2)]
        [DataRow("历战灭尽龙", "灭尽龙", 1)]
        [DataRow("王灭尽龙", "灭尽龙", 2)]
        [DataRow("灭尽龙", "灭尽龙", 0)]
        [DataRow("历战王溟波龙", "溟波龙", 2)]
        [DataRow("历战钢龙", "钢龙", 1)]
        public void NormalizeMonsterName_Parses_Tempered_Level(string input, string expectedName, int expectedLevel)
        {
            var result = NormalizeMonsterName(input);
            Assert.AreEqual(expectedName, result.Item1);
            Assert.AreEqual(expectedLevel, result.Item2);
        }

        [DataTestMethod]
        [DataRow("点怪 灭尽龙", "点怪灭尽龙")]
        [DataRow("点怪，灭尽龙", "点怪灭尽龙")]
        [DataRow("点怪,灭尽龙", "点怪灭尽龙")]
        [DataRow("点怪  ，灭尽龙", "点怪灭尽龙")]
        [DataRow("点怪灭尽龙", "点怪灭尽龙")]
        public void NormalizeString_Removes_Spaces_And_Commas(string input, string expected)
        {
            Assert.AreEqual(expected, NormalizeString(input));
        }

        [TestMethod]
        public void FullParse_NormalOrder_ExtractsMonsterName()
        {
            var text = NormalizeString("点怪灭尽龙");
            Assert.IsTrue(IsOrderMessage(text, out var remainder));
            Assert.AreEqual("灭尽龙", remainder);
            var monsterInfo = NormalizeMonsterName(remainder);
            Assert.AreEqual("灭尽龙", monsterInfo.Item1);
            Assert.AreEqual(0, monsterInfo.Item2);
        }

        [TestMethod]
        public void FullParse_TemperedOrder_ExtractsMonsterAndLevel()
        {
            var text = NormalizeString("点怪历战王溟波龙");
            Assert.IsTrue(IsOrderMessage(text, out var remainder));
            Assert.AreEqual("历战王溟波龙", remainder);
            var monsterInfo = NormalizeMonsterName(remainder);
            Assert.AreEqual("溟波龙", monsterInfo.Item1);
            Assert.AreEqual(2, monsterInfo.Item2);
        }

        [TestMethod]
        public void FullParse_PriorityOrder_ExtractsPriorityAndMonster()
        {
            var text = NormalizeString("点怪优先灭尽龙");
            Assert.IsTrue(IsOrderMessage(text, out var remainder));
            Assert.AreEqual("优先灭尽龙", remainder);
            Assert.IsTrue(HasPriorityKeyword(remainder));
        }

        [TestMethod]
        public void FullParse_NonOrder_ReturnsFalse()
        {
            var text = NormalizeString("大家好我是新来的");
            Assert.IsFalse(IsOrderMessage(text, out _));
        }

        [TestMethod]
        public void FullParse_OrderButNoMonster_ExtractsPrefix()
        {
            var text = NormalizeString("点怪哈哈哈");
            Assert.IsTrue(IsOrderMessage(text, out var remainder));
            Assert.AreEqual("哈哈哈", remainder);
        }
    }
}
