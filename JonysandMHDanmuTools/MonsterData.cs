using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.IO;

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
        private static MonsterData _Inst = null;

        public static MonsterData GetInst()
        {
            if (_Inst != null)
                return _Inst;
            _Inst = new MonsterData();
            return _Inst;
        }

        public bool LoadJsonData()
        {
            try
            {
                bool result = NativeImports.DataBridge_Initialize();
                if (result)
                {
                    string configDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                    configDir = Path.Combine(configDir, "MonsterOrderWilds_configs");
                    MonsterIconLoader.Initialize(configDir);
                }
                return result;
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:DataBridge_Initialize failed: " + e.Message);
                return false;
            }
        }

        public MonsterData()
        {
        }

        public Tuple<string, int> GetMatchedMonsterName(string inputText)
        {
            var nameBuilder = new System.Text.StringBuilder(256);
            int temperedLevel = 0;
            try
            {
                bool matched = NativeImports.DataBridge_MatchMonsterName(inputText, nameBuilder, 256, out temperedLevel);
                if (matched)
                {
                    return new Tuple<string, int>(nameBuilder.ToString(), temperedLevel);
                }
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:DataBridge_MatchMonsterName failed: " + e.Message);
            }
            return new Tuple<string, int>("", 0);
        }

        public string GetMatchedMonsterIconUrl(string monsterName)
        {
            var urlBuilder = new System.Text.StringBuilder(512);
            try
            {
                NativeImports.DataBridge_GetMonsterIconUrl(monsterName, urlBuilder, 512);
                return urlBuilder.ToString();
            }
            catch (Exception e)
            {
                ToolsMain.SendCommand("Log:DataBridge_GetMonsterIconUrl failed: " + e.Message);
            }
            return "";
        }
    }
}