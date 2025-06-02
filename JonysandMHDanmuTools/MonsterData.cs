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
        private Dictionary<string, string> ORDERRABLE_MONSTERS;
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
            if (ORDERRABLE_MONSTERS != null)
                ORDERRABLE_MONSTERS.Clear();
            else
                ORDERRABLE_MONSTERS = new Dictionary<string, string>();
            string configPath = Path.Combine(Environment.CurrentDirectory, @"MonsterOrderWilds_configs", "monster_list.json");
            if (!File.Exists(configPath))
            {
                ToolsMain.SendCommand("Log:Can not find monster list: " + configPath);
                return false;
            }
            string json_string = File.ReadAllText(configPath);
            RawMonsterData = JsonConvert.DeserializeObject<Dictionary<string, OneMonsterData>>(json_string);
            ORDERRABLE_MONSTERS = new Dictionary<string, string>();
            foreach (var item in RawMonsterData)
            {
                foreach (var nick_name in item.Value.别称)
                    ORDERRABLE_MONSTERS["\\b" + nick_name.ToString() + "\\b"] = item.Key;
            }
            return true;
        }

        public MonsterData()
        {
            LoadJsonData();
        }
        
        public Tuple<string, int> GetMatchedMonsterName(string inputText)
        {
            if (ORDERRABLE_MONSTERS == null)
                return new Tuple<string, int>("", 0);
            foreach (var item in ORDERRABLE_MONSTERS)
            {
                Match match = Regex.Match(inputText, item.Key);
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