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
        private Dictionary<string, string> m_recordOrder = new Dictionary<string, string>();

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

            var userName = e.Danmaku.UserName;

            // 是否是重复的用户 
            if (IsRepeatUser(userName))
            {
                return;
            }

            //todo 在这里判怪物名字库

            //todo 打完之后需要回调回来，根据key去remove RecordOrderDic

            if (e.Danmaku.UserGuardLevel > 0)
            {
                switch (e.Danmaku.UserGuardLevel)
                {
                    case 1:
                        {
                            userName += "（总）";
                            break;
                        }
                    case 2:
                        {
                            userName += "（提）";
                            break;
                        }
                    case 3:
                        {
                            userName += "（舰）";
                            break;
                        }
                    default:
                        break;
                }
                
            }
            
            if (e.Danmaku.MsgType == MsgTypeEnum.Comment)
            {
                foreach (var pattern in order_monster_patterns)
                {
                    Match match = Regex.Match(e.Danmaku.CommentText, pattern);
                    if (match.Success)
                    {
                        string monster_name = e.Danmaku.CommentText.Substring(match.Index + 2);
                        monster_name = NormalizeMonsterName(monster_name);
                        
                        if (!CheckMonsterOrderrable(monster_name, e.Danmaku))
                            return;

                        // 记录当前的订单
                        m_recordOrder.Add(userName, monster_name);

                        _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                        {
                            _OrderedMonsterWindow.AddOrder(userName, monster_name);
                        }));
                        break;
                    }
                }
            }
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
            return m_recordOrder.ContainsKey(data);
        }

        private string NormalizeMonsterName(string monster_name)
        {
            monster_name = monster_name.Replace(" ", "")
                .Replace(",", "")
                .Replace("，", "");
            return monster_name;
        }

        // 本怪物是否可点
        private bool CheckMonsterOrderrable(string monster_name, DanmakuModel danmu_info)
        {
            int fans_medal_level = (int)danmu_info.RawDataJToken["data"]["fans_medal_level"];
            string fans_medal_name = danmu_info.RawDataJToken["data"]["fans_medal_name"].ToString();
            _ToolsMain.Log("弹幕粉丝牌：" + fans_medal_name + " 等级 " + fans_medal_level.ToString() + " 舰长状态 " + danmu_info.UserGuardLevel.ToString());
            if (fans_medal_level == 0)
                return false;
            return true;
        }

    }
}
