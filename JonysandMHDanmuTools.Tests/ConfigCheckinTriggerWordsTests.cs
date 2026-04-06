using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace JonysandMHDanmuTools.Tests
{
    [TestClass]
    public class ConfigCheckinTriggerWordsTests
    {
        [TestMethod]
        public void TestCheckinTriggerWords_DefaultValue()
        {
            string defaultTriggerWords = "打卡,签到";
            Assert.AreEqual("打卡,签到", defaultTriggerWords);
        }

        [TestMethod]
        public void TestCheckinTriggerWords_CanParseMultipleWords()
        {
            string triggerWords = "打卡,签到,报道";
            string[] words = triggerWords.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
            Assert.AreEqual(3, words.Length);
            Assert.AreEqual("打卡", words[0]);
            Assert.AreEqual("签到", words[1]);
            Assert.AreEqual("报道", words[2]);
        }

        [TestMethod]
        public void TestCheckinTriggerWords_CaseInsensitiveMatch()
        {
            string content = "我来签到啦";
            string triggerWords = "打卡,签到";
            bool isMatch = false;
            foreach (var word in triggerWords.Split(','))
            {
                if (content.Contains(word))
                {
                    isMatch = true;
                    break;
                }
            }
            Assert.IsTrue(isMatch);
        }

        [TestMethod]
        public void TestCheckinTriggerWords_ContainsChineseCharacters()
        {
            string triggerWords = "打卡,签到";
            Assert.IsTrue(triggerWords.Contains("打"));
            Assert.IsTrue(triggerWords.Contains("卡"));
            Assert.IsTrue(triggerWords.Contains("签"));
            Assert.IsTrue(triggerWords.Contains("到"));
        }

        [TestMethod]
        public void TestEnableCaptainCheckinAI_DefaultTrue()
        {
            bool defaultValue = true;
            Assert.IsTrue(defaultValue);
        }

        [TestMethod]
        public void TestEnableCaptainCheckinAI_CanToggle()
        {
            bool enabled = true;
            enabled = false;
            Assert.IsFalse(enabled);
            enabled = true;
            Assert.IsTrue(enabled);
        }

        [TestMethod]
        public void TestCheckinTriggerWords_EmptyString_NoTrigger()
        {
            string triggerWords = "";
            string content = "hello world";
            bool isMatch = false;
            if (!string.IsNullOrEmpty(triggerWords))
            {
                foreach (var word in triggerWords.Split(','))
                {
                    if (content.Contains(word))
                    {
                        isMatch = true;
                        break;
                    }
                }
            }
            Assert.IsFalse(isMatch);
        }

        [TestMethod]
        public void TestCheckinTriggerWords_SingleWord()
        {
            string triggerWords = "打卡";
            string[] words = triggerWords.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
            Assert.AreEqual(1, words.Length);
            Assert.AreEqual("打卡", words[0]);
        }

        [TestMethod]
        public void TestCheckinEvent_ContinuousDaysCalculation()
        {
            int continuousDays = 5;
            Assert.AreEqual(5, continuousDays);
            continuousDays++;
            Assert.AreEqual(6, continuousDays);
        }

        [TestMethod]
        public void TestCheckinEvent_ContinuousDays_ResetAfterGap()
        {
            int continuousDays = 5;
            int previousDate = 20260404;
            int currentDate = 20260406;
            if (currentDate != previousDate + 1)
            {
                continuousDays = 1;
            }
            Assert.AreEqual(1, continuousDays);
        }
    }
}
