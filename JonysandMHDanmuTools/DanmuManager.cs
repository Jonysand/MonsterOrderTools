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

namespace JonysandMHDanmuTools
{
    internal class DanmuManager
    {
        private static OrderedMonsterWindow _OrderedMonsterWindow = null;

        private string[] order_monster_patterns;
        
        private string[] priority_patterns_withoutOrder;

        // 简易优先队列实现点怪记录
        private PriorityQueue m_queueRecord = new PriorityQueue();
        

        public DanmuManager()
        {
            order_monster_patterns = new string[6] { @"^点怪", @"^点个", @"^点只", 
                                                    @"^點怪", @"^點個", @"^點隻" };
            
            priority_patterns_withoutOrder = new string[4] { @"^优先", @"^插队", @"^優先", @"^插隊" };
        }

        public void SetOrderedMonsterWindow(OrderedMonsterWindow orderedMonsterWindow)
        {
            _OrderedMonsterWindow = orderedMonsterWindow;
        }

        // 收到弹幕的处理
        public void OnReceivedDanmaku(object sender, ReceivedDanmakuArgs e)
        {
            // 如果窗口都没初始化成功,那么其实后面的流程都不需要走了
            if (e == null || e.Danmaku == null || _OrderedMonsterWindow == null || e.Danmaku.RawDataJToken == null)
            {
                return;
            }

            var jsonData = e.Danmaku.RawDataJToken["data"];
            if (jsonData == null)
            {
                return;
            }

            //处理优先逻辑
            var check = false;
            if (e.Danmaku.MsgType == MsgTypeEnum.Comment)
            {
                foreach (var pattern in priority_patterns_withoutOrder)
                {
                    Match match = Regex.Match(e.Danmaku.CommentText, pattern);
                    if (match.Success)
                    {
                        var queue = m_queueRecord;
                        for (int i = 0; i < queue.Count; i++)
                        {
                            if (queue.Queue[i].UserId == e.Danmaku.UserID_long && e.Danmaku.UserGuardLevel > 0 && !queue.Queue[i].Priority)
                            {
                                queue.Queue[i].Priority = true;
                                break;
                            }
                        }

                        queue.SortQueue();
                        RefreshOrder();
                        check = true;
                    }
                }
            }

            if (check)
            {
                return;
            }


            // 是否成功佩戴粉丝牌
            if (!IsWearingMedal(jsonData))
            {
                return;
            }
            
            // 是否是重复的用户 
            if (IsRepeatUser(e.Danmaku.UserID_long))
            {
                return;
            }

            var isPriority = false;

            var monsterName = string.Empty;
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
                                subString = subString.Substring(match.Index + 2);
                            }
                        }
                        monsterName = NormalizeMonsterName(subString);
                        // 在这里判怪物名字库
                        monsterName = MonsterData.GetInst().GetMatchedMonsterName(monsterName);
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
            var timeStamp = GetDanMuTimeStamp(jsonData);
            m_queueRecord.Enqueue(new PriorityQueueNode(e.Danmaku.UserID_long, timeStamp, isPriority, userName, monsterName, e.Danmaku.UserGuardLevel));
            
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

        private bool IsRepeatUser(long data)
        {
            return m_queueRecord.Contains(data);
        }

        private string NormalizeMonsterName(string monster_name)
        {
            monster_name = monster_name.Replace(" ", "")
                .Replace(",", "")
                .Replace("，", "");
            return monster_name;
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
            _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
            {
                _OrderedMonsterWindow.AddOrder(userName, monsterName, m_queueRecord);
            }));
        }

        private void RefreshOrder()
        {
            _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
            {
                _OrderedMonsterWindow.RefreshOrder(m_queueRecord);
            }));
        }

        // 移除记录
        public void RemoveRecord()
        {
            m_queueRecord.Dequeue();
        }
    }
}
