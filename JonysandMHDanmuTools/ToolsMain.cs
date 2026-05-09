using System;
using System.IO;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Windows;
using System.Runtime.InteropServices;


namespace MonsterOrderWindows
{
    public enum ConnectionState
    {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Reconnecting = 3,
        ReconnectFailed = 4
    }

    public enum DisconnectReason
    {
        None = 0,
        NetworkError = 1,
        HeartbeatTimeout = 2,
        ServerClose = 3,
        AuthFailed = 4
    }

    public class ToolsMain
    {
        private ConfigWindow _ConfigWindow = null;
        private OrderedMonsterWindow _OrderedMonsterWindow = null;
        static private ConfigService _Config = null;

        static public bool IsOnlyOrderMonster { get; private set; } = false;
        public static List<string> ManboVoiceList { get; private set; } = new List<string>
        {
            "曼波",
            "撒娇学妹", "广告男声", "网文解说", "说唱小哥", "动漫小新", "萌娃",
            "温和宝爸", "严厉大叔", "傲娇男声", "拽拽馒头", "生活小妙招", "春日甜妹", "清甜女声", "质感男声",
            "东北能哥", "电竞解说", "水果舞曲", "做作夹子音", "感性女生", "和蔼奶奶", "清爽男大", "九小月",
            "硬妹", "大耳小图", "电视广告", "心机御姐", "童话解说", "武则天", "懒小羊", "春节甜妹",
            "动漫海绵", "直率英子", "情感语录", "侠客", "动漫解说", "甜美解说", "太乙", "军事解说",
            "少儿故事", "皇上", "病弱少女", "川妹子", "译制片男II", "龅牙珍珍", "锤子哥", "甜美悦悦",
            "强势妹", "小品艺术家", "生活导师", "播音旁白", "老婆婆", "黛玉", "康康舞曲", "小女孩",
            "渊博小叔", "调皮公主", "电台广播", "娱乐扒妹II", "天津小哥", "暖心学姐", "美小羊", "柜哥",
            "云龙哥", "台湾男生", "直播一姐", "小魔童", "上海阿姨", "悬疑解说", "猴哥", "译制片男",
            "萌娃百科", "文艺男声", "稚气少女", "乒乓解说", "理智姐", "咆哮哥", "舌尖解说", "赛事解说",
            "松弛男声", "可爱女生", "生活主播", "容嬷嬷", "章鱼哥哥", "东北老铁", "科技博主", "歌唱达人",
            "新闻男声", "温柔淑女", "温柔播报", "港普男声2", "幺妹", "紫薇", "樱花小哥", "扒小编",
            "翩翩公子", "广告男声2", "樱桃爷爷", "潮汕大叔", "太白", "纪录片解说", "知性女声", "清冷女声",
            "康定情歌", "清新歌手", "八戒", "佩奇猪", "顾姐", "解说小帅", "台湾女生", "严厉老太",
            "王小也", "如来佛祖", "熊二", "雅痞大叔", "亲切女声", "粤语男声", "宝宝冯", "电子馒头",
            "小青", "心灵鸡汤", "娱乐扒妹", "知识讲解", "小姐姐", "米老哥", "狐狸姐姐", "广西表哥",
            "重庆小伙", "亲切阿姨", "唐小鸭", "佛系馒头", "沉稳男声", "恐怖电影", "靓女", "京腔",
            "英语女王", "游戏解说男", "河南大叔", "温柔女友", "湘普甜甜", "青岛小哥", "语音助手", "快板",
            "港普男声", "TVB女声", "摇滚男生", "活泼女孩", "女少侠", "天线波波", "娱乐播报", "单口相声",
            "促销男声", "情歌王", "甜美女孩", "傲娇大小姐", "养生丽姐", "温迪迪", "女儿国王", "歌唱女王",
            "激扬男声", "病娇少女", "贺岁女娃", "广普", "旅游资讯", "大丫", "霸总", "沉稳解说",
            "樱桃丸子", "古风男主", "派星星", "西安掌柜", "新闻女声", "阳光男生", "官方客服", "和大人",
            "娱乐播报2", "温柔男声", "魅力女友", "阳光少年", "高冷男声", "春日部姐姐", "猴哥说唱", "东厂公公",
            "商务殷语", "文艺女声"
        };

        public void SetOnlyOrderMonsterMode(bool value)
        {
            IsOnlyOrderMonster = value;
        }

        [DllImport("user32.dll")]
        private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [StructLayout(LayoutKind.Sequential)]
        private struct RECT
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }

        /*
            this.PluginAuth = "鬼酒时雨;Hey_Coder";
            this.PluginName = "点怪姬";
            this.PluginVer = "v0.73";
            this.PluginDesc = "弹幕姬插件开发学习中，祝你每天吃饱饱！";
            this.PluginCont = "QQ: 1600402178";
         */

        public void Inited()
        {
            InitConfigHandlers();

            if (_Config == null)
            {
                try
                {
                    _Config = new ConfigService();
                    _Config.LoadConfig();
                }
                catch (Exception e)
                {
                    MessageBox.Show($"加载配置文件失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                }
            }

            try
            {
                DanmuManager.GetInst();
                MonsterData.GetInst().LoadJsonData();
                DanmuManager.GetInst().LoadHistoryOrder();
                string configDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "MonsterOrderWilds_configs");
                MonsterIconLoader.Initialize(configDir);
            }
            catch (Exception e)
            {
                MessageBox.Show($"启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
            }

            if (!IsOnlyOrderMonster)
            {
                try
                {
                    if (!NativeImports.CaptainCheckInModule_Initialize())
                        System.Diagnostics.Debug.WriteLine("[ToolsMain] CaptainCheckInModule_Initialize failed");
                }
                catch (Exception e)
                {
                    System.Diagnostics.Debug.WriteLine($"[ToolsMain] CaptainCheckInModule init failed: {e.Message}");
                }
            }

            try
            {
                _OrderedMonsterWindow = new OrderedMonsterWindow();
                if (_Config != null && _Config.Config != null)
                {
                    _OrderedMonsterWindow.WindowStartupLocation = WindowStartupLocation.Manual;
                    _OrderedMonsterWindow.Left = _Config.Config.TopPos.X;
                    _OrderedMonsterWindow.Top = _Config.Config.TopPos.Y;
                }
            }
            catch (Exception e)
            {
                MessageBox.Show($"点怪窗口启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
            }

            // 事件注册
            GlobalEventListener.AddListener("LOG", (object msg) => SendCommand("Log:" + msg.ToString()));
            GlobalEventListener.AddListener("OrderWindowLocked", (object msg) => OnOrderWindowLocked());
            GlobalEventListener.AddListener("Message", (object msg) => OnOrderWindowLocked());
            GlobalEventListener.AddListener("ConfigChanged", (object msg) => ConfigChanged(msg));
        }

        public void Stop()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _OrderedMonsterWindow.Hide();
                }));
            }
        }

        public void Start()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                }));
            }
        }

        public void ConfigChanged(object msg)
        {
            if (_Config.Config == null) return;
            var message = msg.ToString();
            var parts = message.Split(':');
            if (message == "WindowPosition")
            {
                IntPtr hwnd = new System.Windows.Interop.WindowInteropHelper(_OrderedMonsterWindow).Handle;
                RECT rect;
                GetWindowRect(hwnd, out rect);
                double left = rect.Left;
                double top = rect.Top;
                var source = System.Windows.PresentationSource.FromVisual(_OrderedMonsterWindow);
                if (source != null)
                {
                    double dpiX = source.CompositionTarget.TransformFromDevice.M11;
                    double dpiY = source.CompositionTarget.TransformFromDevice.M22;
                    left *= dpiX;
                    top *= dpiY;
                }
                System.Diagnostics.Debug.WriteLine($"[DEBUG] WindowPosition: left={left}, top={top}");
                _Config.Config.TopPos = new Point(left, top);
                _Config.SaveConfig();
                System.Diagnostics.Debug.WriteLine($"[DEBUG] WindowPosition saved: {_Config.Config.TopPos}");
                return;
            }
            else if (parts.Length < 2)
            {
                return;
            }

            var key = parts[0];
            var value = parts[1];

            if (_configHandlers != null && _configHandlers.TryGetValue(key, out var handler))
            {
                handler(key, value);
            }

            _Config.SaveConfig();
        }

        private Dictionary<string, Action<string, string>> _configHandlers;

        private void InitConfigHandlers()
        {
            _configHandlers = new Dictionary<string, Action<string, string>>
            {
                ["ID_CODE"] = (k, v) => _Config.Config.ID_CODE = v,
                ["MANBO_API_KEY"] = (k, v) => _Config.Config.MANBO_API_KEY = v,
                ["MANBO_VOICE"] = (k, v) => _Config.Config.MANBO_VOICE = v,
                ["ONLY_MEDAL_ORDER"] = (k, v) => _Config.Config.ONLY_MEDAL_ORDER = v == "1",
                ["ENABLE_VOICE"] = (k, v) => _Config.Config.ENABLE_VOICE = v == "1",
                ["SPEECH_RATE"] = (k, v) => { if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) _Config.Config.SPEECH_RATE = val; },
                ["SPEECH_PITCH"] = (k, v) => { if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) _Config.Config.SPEECH_PITCH = val; },
                ["SPEECH_VOLUME"] = (k, v) => { if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) _Config.Config.SPEECH_VOLUME = val; },
                ["ONLY_SPEEK_WEARING_MEDAL"] = (k, v) => _Config.Config.ONLY_SPEEK_WEARING_MEDAL = v == "1",
                ["ONLY_SPEEK_GUARD_LEVEL"] = (k, v) => { if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) _Config.Config.ONLY_SPEEK_GUARD_LEVEL = val; },
                ["ONLY_SPEEK_PAID_GIFT"] = (k, v) => _Config.Config.ONLY_SPEEK_PAID_GIFT = v == "1",
                ["OPACITY"] = (k, v) => {
                    if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) {
                        _Config.Config.OPACITY = val;
                        _OrderedMonsterWindow?.RefreshWindow();
                    }
                },
                ["PENETRATING_MODE_OPACITY"] = (k, v) => {
                    if (int.TryParse(v, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int val)) {
                        _Config.Config.PENETRATING_MODE_OPACITY = val;
                        _OrderedMonsterWindow?.RefreshWindow();
                    }
                },
                ["TTS_ENGINE"] = (k, v) => _Config.Config.TTS_ENGINE = v,
                ["MIMO_VOICE"] = (k, v) => _Config.Config.MIMO_VOICE = v,
                ["MIMO_STYLE"] = (k, v) => _Config.Config.MIMO_STYLE = v,
                ["MIMO_API_KEY"] = (k, v) => _Config.Config.MIMO_API_KEY = v,
                ["DEFAULT_MARQUEE_TEXT"] = (k, v) => {
                    _Config.Config.DEFAULT_MARQUEE_TEXT = v;
                    GlobalEventListener.Invoke("MarqueeTextChanged", v);
                },
                ["ENABLE_CAPTAIN_CHECKIN_AI"] = (k, v) => _Config.Config.ENABLE_CAPTAIN_CHECKIN_AI = v == "1",
                ["CHECKIN_TRIGGER_WORDS"] = (k, v) => _Config.Config.CHECKIN_TRIGGER_WORDS = v,
            };
        }

        public void OnOrderWindowLocked()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.OnHotKeyLock();
            }
            _Config.SaveConfig();
        }

        // New interface from MonsterOrderWilds ------------------------------------------------------------------------------------------
        public void OpenConfigWindow()
        {
            if (_ConfigWindow == null)
            {
                _ConfigWindow = new ConfigWindow();
                _ConfigWindow.FillConfig(_Config.GetConfig());
                _ConfigWindow.InitLockButtonState(_OrderedMonsterWindow.IsLocked);
                _OrderedMonsterWindow.LockStateChanged += _ConfigWindow.OnLockStateChanged;
            }
            _ConfigWindow.Show();
        }

        public void OnConnected()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                }));
            }
            if (_ConfigWindow != null)
            {
                _ConfigWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _ConfigWindow.SetStatus(ConnectionState.Connected, DisconnectReason.None);
                }));
            }
        }

        public void OnDisconnected()
        {
            if (_ConfigWindow != null)
            {
                _ConfigWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _ConfigWindow.SetStatus(ConnectionState.Disconnected, DisconnectReason.None);
                }));
            }
        }

        public void OnConnectionStateChanged(int state, int reason)
        {
            if (_ConfigWindow != null)
            {
                _ConfigWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _ConfigWindow.SetStatus((ConnectionState)state, (DisconnectReason)reason);
                }));
            }
        }

        public void OnHotKeyLock()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.InvokeAsync(new Action(delegate
                {
                    _OrderedMonsterWindow.OnHotKeyLock();
                }));
            }
        }

        static public ConcurrentQueue<String> CommandQueue = new ConcurrentQueue<String>();
        static public void SendCommand(String message)
        {
            CommandQueue.Enqueue(message);
        }
        public String GetCommand()
        {
            if (CommandQueue.TryDequeue(out String result))
                return result;
            return "";
        }

        public bool RefreshMonsterList()
        {
            return MonsterData.GetInst().LoadJsonData();
        }

        static public ConfigService GetConfigService()
        {
            return _Config;
        }

        public OrderedMonsterWindow GetOrderedMonsterWindow()
        {
            return _OrderedMonsterWindow;
        }

        public void SetWindowVersion(int version)
        {
            if (_ConfigWindow == null) return;
            _ConfigWindow.SetVersion(version);
        }
    }
}
