using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class MonsterDataTests
    {
        private readonly List<KeyValuePair<Regex, string>> _compiledPatterns;
        private readonly Dictionary<string, MockMonsterData> _rawData;

        public class MockMonsterData
        {
            public string 图标地址 { get; set; } = "";
            public List<string> 别称 { get; set; } = new List<string>();
            public int 默认历战等级 { get; set; } = 0;
        }

        public MonsterDataTests()
        {
            _rawData = new Dictionary<string, MockMonsterData>
            {
                ["灭尽龙"] = new MockMonsterData
                {
                    图标地址 = "https://example.com/em103.png",
                    别称 = new List<string> { "灭尽", "灭尽龙", "咩咩子" },
                    默认历战等级 = 0
                },
                ["冥灯龙"] = new MockMonsterData
                {
                    图标地址 = "https://example.com/em105.png",
                    别称 = new List<string> { "小明", "冥灯", "冥灯龙" },
                    默认历战等级 = 0
                },
                ["雪花沉睡"] = new MockMonsterData
                {
                    图标地址 = "https://example.com/em124.png",
                    别称 = new List<string> { "王冰", "王冰呪龙", "雪花沉睡" },
                    默认历战等级 = 2
                },
                ["战痕黑狼鸟"] = new MockMonsterData
                {
                    图标地址 = "https://example.com/em018.png",
                    别称 = new List<string> { "战痕黑鸡", "战痕", "战痕黑狼鸟" },
                    默认历战等级 = 1
                }
            };

            _compiledPatterns = new List<KeyValuePair<Regex, string>>();
            foreach (var item in _rawData)
            {
                foreach (var nick in item.Value.别称)
                {
                    var regex = new Regex("\\b" + nick + "\\b", RegexOptions.Compiled);
                    _compiledPatterns.Add(new KeyValuePair<Regex, string>(regex, item.Key));
                }
            }
        }

        private Tuple<string, int> GetMatchedMonsterName(string inputText)
        {
            foreach (var item in _compiledPatterns)
            {
                Match match = item.Key.Match(inputText);
                if (match.Success)
                {
                    var temperedLevel = _rawData[item.Value].默认历战等级;
                    return new Tuple<string, int>(item.Value, temperedLevel);
                }
            }
            return new Tuple<string, int>("", 0);
        }

        [DataTestMethod]
        [DataRow("灭尽龙", "灭尽龙", 0)]
        [DataRow("灭尽", "灭尽龙", 0)]
        [DataRow("咩咩子", "灭尽龙", 0)]
        [DataRow("冥灯龙", "冥灯龙", 0)]
        [DataRow("小明", "冥灯龙", 0)]
        [DataRow("冥灯", "冥灯龙", 0)]
        public void Match_Finds_Correct_Monster_By_Alias(string input, string expectedName, int expectedLevel)
        {
            var result = GetMatchedMonsterName(input);
            Assert.AreEqual(expectedName, result.Item1);
            Assert.AreEqual(expectedLevel, result.Item2);
        }

        [DataTestMethod]
        [DataRow("雪花沉睡", "雪花沉睡", 2)]
        [DataRow("王冰", "雪花沉睡", 2)]
        [DataRow("王冰呪龙", "雪花沉睡", 2)]
        [DataRow("战痕黑狼鸟", "战痕黑狼鸟", 1)]
        [DataRow("战痕黑鸡", "战痕黑狼鸟", 1)]
        public void Match_Returns_Correct_Default_Tempered_Level(string input, string expectedName, int expectedLevel)
        {
            var result = GetMatchedMonsterName(input);
            Assert.AreEqual(expectedName, result.Item1);
            Assert.AreEqual(expectedLevel, result.Item2);
        }

        [DataTestMethod]
        [DataRow("不存在的怪物")]
        [DataRow("hello world")]
        [DataRow("")]
        public void Match_Returns_Empty_For_Unmatched(string input)
        {
            var result = GetMatchedMonsterName(input);
            Assert.AreEqual("", result.Item1);
            Assert.AreEqual(0, result.Item2);
        }

        [TestMethod]
        public void Match_WordBoundary_Partial_Name_Does_Not_Match_Longer_Name()
        {
            var result = GetMatchedMonsterName("灭");
            Assert.AreEqual("", result.Item1);
        }

        [TestMethod]
        public void Match_WordBoundary_Full_Name_Matches()
        {
            var result = GetMatchedMonsterName("灭尽龙");
            Assert.AreEqual("灭尽龙", result.Item1);
        }

        [TestMethod]
        public void MonsterData_Json_Roundtrip()
        {
            var original = new Dictionary<string, MockMonsterData>
            {
                ["灭尽龙"] = new MockMonsterData
                {
                    图标地址 = "https://example.com/em103.png",
                    别称 = new List<string> { "灭尽", "灭尽龙" },
                    默认历战等级 = 0
                }
            };

            var json = Newtonsoft.Json.JsonConvert.SerializeObject(original);
            var deserialized = Newtonsoft.Json.JsonConvert.DeserializeObject<Dictionary<string, MockMonsterData>>(json);

            Assert.IsNotNull(deserialized);
            Assert.IsTrue(deserialized.ContainsKey("灭尽龙"));
            Assert.AreEqual(2, deserialized["灭尽龙"].别称.Count);
            Assert.AreEqual("https://example.com/em103.png", deserialized["灭尽龙"].图标地址);
        }
    }
}
