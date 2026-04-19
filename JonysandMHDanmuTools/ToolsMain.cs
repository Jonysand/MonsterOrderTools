using System;
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
            }
            catch (Exception e)
            {
                MessageBox.Show($"启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
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
            GlobalEventListener.AddListener("LOG", (object msg) => SendCommand("LOG:" + msg.ToString()));
            GlobalEventListener.AddListener("OrderWindowLocked", (object msg) => OnOrderWindowLocked());
            GlobalEventListener.AddListener("Message", (object msg) => OnOrderWindowLocked());
            GlobalEventListener.AddListener("ConfigChanged", (object msg) => ConfigChanged(msg));
        }

        public void Stop()
        {
            try
            {
                NativeImports.DataBridge_Shutdown();
            }
            catch (Exception e)
            {
                SendCommand("Log:DataBridge_Shutdown failed: " + e.Message);
            }

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
                ["ONLY_MEDAL_ORDER"] = (k, v) => _Config.Config.ONLY_MEDAL_ORDER = v == "1",
                ["ENABLE_VOICE"] = (k, v) => _Config.Config.ENABLE_VOICE = v == "1",
                ["SPEECH_RATE"] = (k, v) => { if (int.TryParse(v, out int val)) _Config.Config.SPEECH_RATE = val; },
                ["SPEECH_PITCH"] = (k, v) => { if (int.TryParse(v, out int val)) _Config.Config.SPEECH_PITCH = val; },
                ["SPEECH_VOLUME"] = (k, v) => { if (int.TryParse(v, out int val)) _Config.Config.SPEECH_VOLUME = val; },
                ["ONLY_SPEEK_WEARING_MEDAL"] = (k, v) => _Config.Config.ONLY_SPEEK_WEARING_MEDAL = v == "1",
                ["ONLY_SPEEK_GUARD_LEVEL"] = (k, v) => { if (int.TryParse(v, out int val)) _Config.Config.ONLY_SPEEK_GUARD_LEVEL = val; },
                ["ONLY_SPEEK_PAID_GIFT"] = (k, v) => _Config.Config.ONLY_SPEEK_PAID_GIFT = v == "1",
                ["OPACITY"] = (k, v) => {
                    if (int.TryParse(v, out int val)) {
                        _Config.Config.OPACITY = val;
                        _OrderedMonsterWindow?.RefreshWindow();
                    }
                },
                ["PENETRATING_MODE_OPACITY"] = (k, v) => {
                    if (int.TryParse(v, out int val)) {
                        _Config.Config.PENETRATING_MODE_OPACITY = val;
                        _OrderedMonsterWindow?.RefreshWindow();
                    }
                },
                ["TTS_ENGINE"] = (k, v) => _Config.Config.TTS_ENGINE = v,
                ["MINIMAX_VOICE_ID"] = (k, v) => _Config.Config.MINIMAX_VOICE_ID = v,
                ["MINIMAX_SPEED"] = (k, v) => { if (float.TryParse(v, out float val)) _Config.Config.MINIMAX_SPEED = val; },
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
                GlobalEventListener.AddListener("ConfigChanged", (object msg) => ConfigChanged(msg));
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

        static public Queue<String> CommandQueue;
        static public void SendCommand(String message)
        {
            if (CommandQueue == null)
                CommandQueue = new Queue<String>();
            CommandQueue.Enqueue(message);
        }
        public String GetCommand()
        {
            if (CommandQueue == null)
                CommandQueue = new Queue<String>();
            if (CommandQueue.Count > 0)
                return CommandQueue.Dequeue();
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
