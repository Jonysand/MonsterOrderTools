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

namespace JonysandMHDanmuTools
{
    internal class DanmuManager
    {
        private static ToolsMain _ToolsMain = null;
        private static OrderedMonsterWindow _OrderedMonsterWindow = null;

        private string[] order_monster_patterns;

        // 点怪记录
        private Queue<long> m_recordUserID = new Queue<long>();

        public DanmuManager(ToolsMain toolsMain)
        {
            _ToolsMain = toolsMain;
            order_monster_patterns = new string[6] { @"^点怪", @"^点个", @"^点只", 
                                                    @"^點怪", @"^點個", @"^點隻" };
        }

        public void SetOrderedMonsterWindow(OrderedMonsterWindow orderedMonsterWindow)
        {
            _OrderedMonsterWindow = orderedMonsterWindow;
        }

        // 收到弹幕的处理
        public void OnReceivedDanmaku(object sender, ReceivedDanmakuArgs e)
        {
            // 如果窗口都没初始化成功,那么其实后面的流程都不需要走了
            if (_OrderedMonsterWindow == null)
            {
                return;
            }

            var jsonData = e.Danmaku.RawDataJToken["data"];
            if (jsonData == null)
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

            //todo 在这里判怪物名字库
            var monsterName = string.Empty;
            if (e.Danmaku.MsgType == MsgTypeEnum.Comment)
            {
                foreach (var pattern in order_monster_patterns)
                {
                    Match match = Regex.Match(e.Danmaku.CommentText, pattern);
                    if (match.Success)
                    {
                        monsterName = e.Danmaku.CommentText.Substring(match.Index + 2);
                        monsterName = NormalizeMonsterName(monsterName);
                    }
                }
            }

            var userName = e.Danmaku.UserName;
            if (e.Danmaku.UserGuardLevel > 0)
            {
                switch (e.Danmaku.UserGuardLevel)
                {
                    case 1:
                    {
                        userName += "（总督）";
                        break;
                    }
                    case 2:
                    {
                        userName += "（提督）";
                        break;
                    }
                    case 3:
                    {
                        userName += "（舰长）";
                        break;
                    }
                    default:
                        break;
                }

            }

            //todo 处理优先问题，实际是排序

            // 记录当前的订单
            m_recordUserID.Enqueue(e.Danmaku.UserID_long);
            // 创建订单
            CreateOrder(userName, monsterName);
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
            return m_recordUserID.Contains(data);
        }

        private string NormalizeMonsterName(string monster_name)
        {
            monster_name = monster_name.Replace(" ", "")
                .Replace(",", "")
                .Replace("，", "");
            return monster_name;
        }

        private void CreateOrder(string userName, string monsterName)
        {
            _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
            {
                _OrderedMonsterWindow.AddOrder(userName, monsterName);
            }));
        }

        // 移除记录
        public void RemoveRecord()
        {
            m_recordUserID.Dequeue();
        }
    }
}
