using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using BilibiliDM_PluginFramework;
using System.Windows.Interop;
using System.Linq.Expressions;
using Newtonsoft.Json.Linq;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.StartPanel;
using System.Reflection;
using System.Windows.Media;
using System.Collections;
using System.Data.SqlTypes;

namespace JonysandMHDanmuTools
{
    internal class DanmuManager
    {
        private string[] order_monster_patterns;
        
        private string[] priority_patterns_withoutOrder;

        private string[] separate_priority_patterns_withoutOrder;

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

            // separate_priority_patterns_withoutOrder = new string[4] { @"^优先+$", @"^插队+$", @"^優先+$", @"^插隊+$" };
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

        // 收到弹幕的处理
        public void OnReceivedDanmaku(object sender, ReceivedDanmakuArgs e)
        {
            // 如果窗口都没初始化成功,那么其实后面的流程都不需要走了
            if (e == null || e.Danmaku == null ||  e.Danmaku.RawDataJToken == null)
            {
                return;
            }

            var jsonData = e.Danmaku.RawDataJToken["data"];
            if (jsonData == null || e.Danmaku.MsgType != MsgTypeEnum.Comment)
            {
                return;
            }

            // 是否成功佩戴粉丝牌
            if (!IsWearingMedal(jsonData))
            {
                return;
            }

            // 阿b把uid给去了，真的服
            var open_id_obj = jsonData["open_id"];
            if (open_id_obj == null)
                return;
            string open_id = open_id_obj.ToString();

            var timeStamp = GetDanMuTimeStamp(jsonData);
            var isPriority = false;

            //处理优先逻辑
            var check = false;
            foreach (var pattern in priority_patterns_withoutOrder)
            {
                Match match = Regex.Match(e.Danmaku.CommentText, pattern);
                if (match.Success)
                {
                    isPriority = true;
                    for (int i = 0; i < PriorityQueue.GetInst().Count; i++)
                    {
                        if (PriorityQueue.GetInst().Queue[i].UserId == open_id && e.Danmaku.UserGuardLevel > 0)
                        {
                            // 只插队一次
                            if (PriorityQueue.GetInst().Queue[i].Priority != 0)
                                break;
                            PriorityQueue.GetInst().Queue[i].Priority = e.Danmaku.UserGuardLevel;
                            PriorityQueue.GetInst().Queue[i].TimeStamp = timeStamp;
                            break;
                        }
                    }
                    GlobalEventListener.Invoke("RefreshOrder", null);
                    check = true;
                }
            }

            if (check)
            {
                return;
            }
            
            // 是否是重复的用户 
            if (IsRepeatUser(open_id))
            {
                return;
            }

            var monsterName = string.Empty;
            int temperedLevel = 0;
            if (e.Danmaku.MsgType == MsgTypeEnum.Comment)
            {
                var msg = e.Danmaku.CommentText;
                msg = NormalizeString(msg);
                foreach (var pattern in order_monster_patterns)
                {
                    // 点怪规则匹配
                    Match match = Regex.Match(msg, pattern);
                    if (match.Success)
                    {
                        // 插队规则匹配
                        var subString = msg.Substring(match.Index + 2);
                        foreach (var priority in priority_patterns_withoutOrder)
                        {
                            var priorityMatch = Regex.Match(subString, priority);
                            
                            if (priorityMatch.Success && e.Danmaku.UserGuardLevel > 0)
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
                {
                    return;
                }
            }
            else
                return;

            var userName = e.Danmaku.UserName;
            if (e.Danmaku.UserGuardLevel > 0)
            {
                switch (e.Danmaku.UserGuardLevel)
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
            oneNode.UserId = open_id;
            oneNode.TimeStamp = timeStamp;
            oneNode.Priority = isPriority ? e.Danmaku.UserGuardLevel : 0;
            oneNode.UserName = userName;
            oneNode.MonsterName = monsterName;
            oneNode.GuardLevel = e.Danmaku.UserGuardLevel;
            oneNode.TemperedLevel = temperedLevel;
            PriorityQueue.GetInst().Enqueue(oneNode);
            
            // 创建订单
            CreateOrder(userName, monsterName);
        }

        public void OnReceivedRoomCount(object sender, ReceivedRoomCountArgs e)
        {
        }

        private bool IsWearingMedal(JToken data)
        {
            if (data["fans_medal_wearing_status"] != null)
            {
                var status = data["fans_medal_wearing_status"].ToObject<bool>();
                return status;
            }

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

        private long GetDanMuTimeStamp(JToken data)
        {
            if (data["fans_medal_wearing_status"] != null)
            {
                var timeStamp = data["timestamp"].ToObject<long>();
                return timeStamp;
            }

            return -1;
        }

        private void CreateOrder(string userName, string monsterName)
        {
            GlobalEventListener.Invoke("RefreshOrder", null);
            GlobalEventListener.Invoke("AddRollingInfo", new RollingInfo(userName + " 点怪 " + monsterName + " 成功！", Colors.Yellow));
        }
    }
}
