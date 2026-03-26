using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.IO;
using Newtonsoft.Json;

namespace MonsterOrderWindows
{
    public class OneMonsterData
    {
        public string 图标地址 { get; set; } = "";
        public List<string> 别称 { get; set; } = new List<string>();
        public int 默认历战等级 { get; set; } = 0;
    }
    internal class MonsterData
    {
        private List<KeyValuePair<Regex, string>> _compiledPatterns;
        private Dictionary<string, OneMonsterData> RawMonsterData;
        private static MonsterData _Inst = null;

        // Singleton
        public static MonsterData GetInst()
        {
            if (_Inst != null)
                return _Inst;
            _Inst = new MonsterData();
            return _Inst;
        }

        public bool LoadJsonData()
        {
            _compiledPatterns = new List<KeyValuePair<Regex, string>>();
            string configPath = Path.Combine(Environment.CurrentDirectory, @"MonsterOrderWilds_configs", "monster_list.json");
            if (!File.Exists(configPath))
            {
                ToolsMain.SendCommand("Log:Can not find monster list: " + configPath);
                return false;
            }
            string json_string = File.ReadAllText(configPath);
            RawMonsterData = JsonConvert.DeserializeObject<Dictionary<string, OneMonsterData>>(json_string);
            if (RawMonsterData == null)
            {
                RawMonsterData = new Dictionary<string, OneMonsterData>();
                return false;
            }
            foreach (var item in RawMonsterData)
            {
                foreach (var nick_name in item.Value.别称)
                {
                    var regex = new Regex("\\b" + nick_name + "\\b", RegexOptions.Compiled);
                    _compiledPatterns.Add(new KeyValuePair<Regex, string>(regex, item.Key));
                }
            }
            return true;
        }

        public MonsterData()
        {
            LoadJsonData();
        }
        
        public Tuple<string, int> GetMatchedMonsterName(string inputText)
        {
            if (_compiledPatterns == null)
                return new Tuple<string, int>("", 0);
            foreach (var item in _compiledPatterns)
            {
                Match match = item.Key.Match(inputText);
                if (match.Success)
                {
                    var temperedLevel = RawMonsterData[item.Value].默认历战等级;
                    return new Tuple<string, int>(item.Value, temperedLevel);
                }
            }
            return new Tuple<string, int>("", 0);
        }

        public string GetMatchedMonsterIconUrl(string monsterName)
        {
            if (RawMonsterData.ContainsKey(monsterName))
                return RawMonsterData[monsterName].图标地址;
            return "";
        }
    }
}