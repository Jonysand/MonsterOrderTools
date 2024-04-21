using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using Newtonsoft.Json;
using System.IO;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Newtonsoft.Json.Linq;

namespace JonysandMHDanmuTools
{
    internal class MonsterData
    {
        private Dictionary<string, string> ORDERRABLE_MONSTERS;
        private JObject RawMonsterData;
        private static MonsterData _Inst = null;

        // Singleton
        public static MonsterData GetInst()
        {
            if (_Inst != null)
                return _Inst;
            _Inst = new MonsterData();
            return _Inst;
        }

        private bool LoadJsonData()
        {
            string configPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal), @"弹幕姬\plugins\MonsterOrder", "点怪名单.json");
            if (!File.Exists(configPath))
                return false;
            string json_string = File.ReadAllText(configPath);
            RawMonsterData = JObject.Parse(json_string);

            return true;
        }

        public MonsterData()
        {
            if (!LoadJsonData())
                return;
            ORDERRABLE_MONSTERS = new Dictionary<string, string>();
            foreach (var item in RawMonsterData)
            {
                foreach (var nick_name in item.Value["别称"])
                    ORDERRABLE_MONSTERS["\\b" + nick_name.ToString() + "\\b"] = item.Key;
            }
        }
        
        public Tuple<string, int> GetMatchedMonsterName(string inputText)
        {
            foreach (var item in ORDERRABLE_MONSTERS)
            {
                Match match = Regex.Match(inputText, item.Key);
                if (match.Success)
                {
                    var temperedLevel = (int)RawMonsterData[item.Value]["默认历战等级"];
                    return new Tuple<string, int>(item.Value, temperedLevel);
                }
            }
            return new Tuple<string, int>("", 0);
        }

        public string GetMatchedMonsterIconUrl(string monsterName)
        {
            if (RawMonsterData.ContainsKey(monsterName))
                return RawMonsterData[monsterName]["图标地址"].ToString();
            return "";
        }
    }
}