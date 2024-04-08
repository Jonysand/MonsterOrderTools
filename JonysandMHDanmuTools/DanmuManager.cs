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
        public void OnReceivedDanmaku(object sender, BilibiliDM_PluginFramework.ReceivedDanmakuArgs e)
        {
            string danmu_user = e.Danmaku.UserName;
            if (e.Danmaku.UserGuardLevel > 0)
            {
                switch (e.Danmaku.UserGuardLevel)
                {
                    case 1:
                        {
                            danmu_user = danmu_user + "（总）";
                            break;
                        }
                    case 2:
                        {
                            danmu_user = danmu_user + "（提）";
                            break;
                        }
                    case 3:
                        {
                            danmu_user = danmu_user + "（舰）";
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
                        if (_OrderedMonsterWindow == null)
                            return;
                        _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                        {
                            _OrderedMonsterWindow.AddOrder(danmu_user, monster_name);
                        }));
                        break;
                    }
                }
            }
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
