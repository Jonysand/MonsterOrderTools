using System;
using System.Text.RegularExpressions;
using System.Windows.Media;
using Newtonsoft.Json;

namespace MonsterOrderWindows
{
    internal class DanmuManager
    {
        private string[] order_monster_patterns;
        
        private string[] priority_patterns_withoutOrder;

        private string medalName = "";

        static DanmuManager _Inst = null;

        public static DanmuManager GetInst()
        {
            if (_Inst == null)
                _Inst = new DanmuManager();
            return _Inst;
        }

        public DanmuManager()
        {
            order_monster_patterns = new string[6] { @"^点怪", @"^点个", @"^点只", 
                                                    @"^點怪", @"^點個", @"^點隻" };
            
            priority_patterns_withoutOrder = new string[4] { @"优先", @"插队", @"優先", @"插隊" };
        }

        public void LoadHistoryOrder()
        {
            PriorityQueue.GetInst().LoadList();
            GlobalEventListener.Invoke("RefreshOrder", null);
        }

        public void ClearHistoryOrder()
        {
            PriorityQueue.GetInst().Clear();
            GlobalEventListener.Invoke("RefreshOrder", null);
        }


        private class Danmu
        {
            public string cmd { get; set; } = "";
            public class DanmuData
            {
                public string open_id { get; set; } = "";
                public string fans_medal_name { get; set; } = "";
                public int fans_medal_level { get; set; } = 0;
                public bool fans_medal_wearing_status { get; set; } = false;
                public long timestamp { get; set; } = 0;
                public string msg { get; set; } = "";
                public int guard_level { get; set; } = 0;
                public string uname { get; set; } = "";
            }
            public DanmuData data { get; set; } = new DanmuData();
        }
        // 收到弹幕的处理
        public void OnReceicedRawJson(String rawJson)
        {
            Danmu danmu = JsonConvert.DeserializeObject<Danmu>(rawJson);
            if (danmu.cmd != "LIVE_OPEN_PLATFORM_DM")
                return;
            if (danmu.data.open_id.Length == 0)
                return;
            // 是否成功佩戴粉丝牌
            if (!IsWearingMedal(danmu.data))
                return;
            var timeStamp = danmu.data.timestamp;
            var isPriority = false;
            //处理优先逻辑
            var check = false;
            String CommentText = danmu.data.msg;
            int UserGuardLevel = danmu.data.guard_level;
            foreach (var pattern in priority_patterns_withoutOrder)
            {
                Match match = Regex.Match(CommentText, pattern);
                if (match.Success)
                {
                    isPriority = true;
                    for (int i = 0; i < PriorityQueue.GetInst().Count; i++)
                    {
                        if (PriorityQueue.GetInst().Queue[i].UserId == danmu.data.open_id && UserGuardLevel > 0)
                        {
                            // 只插队一次
                            if (PriorityQueue.GetInst().Queue[i].Priority)
                                break;
                            // 插队成功后重置时间戳
                            PriorityQueue.GetInst().Queue[i].Priority = true;
                            PriorityQueue.GetInst().Queue[i].TimeStamp = timeStamp;
                            check = true;
                            break;
                        }
                    }
                    if (check)
                    {
                        GlobalEventListener.Invoke("RefreshOrder", null);
                        return;
                    }
                }
            }

            // 是否是重复的用户 
            if (IsRepeatUser(danmu.data.open_id))
                return;

            var monsterName = string.Empty;
            int temperedLevel = 0;
            CommentText = NormalizeString(CommentText);
            foreach (var pattern in order_monster_patterns)
            {
                // 点怪规则匹配
                Match match = Regex.Match(CommentText, pattern);
                if (match.Success)
                {
                    // 插队规则匹配
                    var subString = CommentText.Substring(match.Index + 2);
                    foreach (var priority in priority_patterns_withoutOrder)
                    {
                        var priorityMatch = Regex.Match(subString, priority);

                        if (priorityMatch.Success && UserGuardLevel > 0)
                        {
                            isPriority = true;
                            subString = subString.Replace(subString.Substring(priorityMatch.Index, priority.Length), "");
                        }
                    }
                    // 在这里判怪物名字库
                    var real_monster_names = MonsterData.GetInst().GetMatchedMonsterName(subString);
                    // 如果直接匹配到，直接用，一般是特殊任务
                    if (!string.IsNullOrEmpty(real_monster_names.Item1))
                    {
                        monsterName = real_monster_names.Item1;
                        temperedLevel = real_monster_names.Item2;
                    }
                    else
                    {
                        var monster_names = NormalizeMonsterName(subString);
                        monsterName = monster_names.Item1;
                        temperedLevel = monster_names.Item2;
                        real_monster_names = MonsterData.GetInst().GetMatchedMonsterName(monsterName);
                        if (real_monster_names.Item2 > 0)
                            temperedLevel = real_monster_names.Item2;
                        monsterName = real_monster_names.Item1;
                    }
                    if (string.IsNullOrEmpty(monsterName))
                    {
                        continue;
                    }
                    break;
                }
            }

            if (string.IsNullOrEmpty(monsterName))
                return;
            var userName = danmu.data.uname;
            if (UserGuardLevel > 0)
            {
                switch (UserGuardLevel)
                {
                    case 1:
                        {
                            userName += "[总督]";
                            break;
                        }
                    case 2:
                        {
                            userName += "[提督]";
                            break;
                        }
                    case 3:
                        {
                            userName += "[舰长]";
                            break;
                        }
                    default:
                        break;
                }

            }

            //记录当前的订单
            var oneNode = new PriorityQueueNode();
            oneNode.UserId = danmu.data.open_id;
            oneNode.TimeStamp = timeStamp;
            oneNode.Priority = isPriority;
            oneNode.UserName = userName;
            oneNode.MonsterName = monsterName;
            oneNode.GuardLevel = UserGuardLevel;
            oneNode.TemperedLevel = temperedLevel;
            PriorityQueue.GetInst().Enqueue(oneNode);

            // 创建订单
            CreateOrder(userName, monsterName);
        }

        public void SetMedalName(string name)
        {
            medalName = name;
        }

        private bool IsWearingMedal(Danmu.DanmuData data)
        {
            if (medalName.Length == 0)
                return true;
            if (data.fans_medal_name.Length > 0)
                return data.fans_medal_name.Equals(medalName) && data.fans_medal_level > 0;
            return false;
        }

        private bool IsRepeatUser(string data)
        {
            return PriorityQueue.GetInst().Contains(data);
        }

        // 返回：（怪物名，历战等级）
        // 曙光的话应该还能用来标记怪异等级，到时候再说
        private Tuple<string, int> NormalizeMonsterName(string monster_name)
        {
            int temperedLevel;
            // 历战等级设置
            if (Regex.Match(monster_name, @"^历战王").Success)
            {
                temperedLevel = 2;
                monster_name = monster_name.Substring(3);
            }
            else if ((Regex.Match(monster_name, @"^历战").Success))
            {
                temperedLevel = 1;
                monster_name = monster_name.Substring(2);
            }
            else if ((Regex.Match(monster_name, @"^王").Success))
            {
                temperedLevel = 2;
                monster_name = monster_name.Substring(1);
            }
            else
                temperedLevel = 0;
            return new Tuple<string, int>(monster_name, temperedLevel);
        }

        private string NormalizeString(string data)
        {
            data = data.Replace(" ", "").Replace(",", "").Replace("，", "");
            return data;
        }

        private void CreateOrder(string userName, string monsterName)
        {
            GlobalEventListener.Invoke("RefreshOrder", null);
            GlobalEventListener.Invoke("AddRollingInfo", new RollingInfo(userName + " 点怪 " + monsterName + " 成功！", Colors.Yellow));
        }
    }
}
