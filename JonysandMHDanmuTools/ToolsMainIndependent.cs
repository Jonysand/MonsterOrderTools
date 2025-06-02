using BilibiliDM_PluginFramework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;


namespace JonysandMHDanmuTools
{
    // public class ToolsMainIndependent : BilibiliDM_PluginFramework.DMPlugin
    public class ToolsMainIndependent
    {
        private ConfigWindow _ConfigWindow = null;
        private OrderedMonsterWindow _OrderedMonsterWindow = null;
        private ConfigService _Config = null;

        public ToolsMainIndependent()
        {
            this.Connected += OnConnected;
            this.Disconnected += OnDisconnected;
            this.ReceivedDanmaku += OnReceivedDanmaku;
            this.ReceivedRoomCount += OnReceivedRoomCount;
            this.PluginVer = "v0.73";
        }

        private void OnReceivedRoomCount(object sender, BilibiliDM_PluginFramework.ReceivedRoomCountArgs e)
        {
            DanmuManager.GetInst().OnReceivedRoomCount(sender, e);
        }

        private void OnDisconnected(object sender, BilibiliDM_PluginFramework.DisconnectEvtArgs e)
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Hide();
                }));
            }
        }

        private void OnReceivedDanmaku(object sender, BilibiliDM_PluginFramework.ReceivedDanmakuArgs e)
        {
            DanmuManager.GetInst().OnReceivedDanmaku(sender, e);
        }

        public void Inited()
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
            try
            {
                _OrderedMonsterWindow = new OrderedMonsterWindow();
                if (_Config != null)
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

            try
            {
                DanmuManager.GetInst();
            }
            catch (Exception e)
            {
                MessageBox.Show($"启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
            }

            // 事件注册
            GlobalEventListener.AddListener("LOG", (object msg) => Log(msg.ToString()));
            GlobalEventListener.AddListener("ConfigChanged", (object msg) => ConfigChanged(msg));
            GlobalEventListener.AddListener("OrderWindowLocked", (object msg) => OnOrderWindowLocked());
        }

        private void OnConnected(object sender, BilibiliDM_PluginFramework.ConnectedEvtArgs e)
        {
            if (_OrderedMonsterWindow != null && this.Status)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                }));
            }
        }

        public override void Admin()
        {
            base.Admin();

            if (_ConfigWindow == null)
                _ConfigWindow = new ConfigWindow();
            _ConfigWindow.Show();
        }

        public override void Stop()
        {
            base.Stop();
            //請勿使用任何阻塞方法
            this.Log("Plugin Stoped!");

            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Hide();
                }));
            }
        }

        public override void Start()
        {
            base.Start();
            //請勿使用任何阻塞方法
            this.Log("Plugin Started!");

            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                }));
            }
        }

        public void ConfigChanged(object msg)
        {
            var message = msg.ToString();
            if (message == "WindowPosition")
            {
                double top = _OrderedMonsterWindow.Top;
                double left = _OrderedMonsterWindow.Left;
                _Config.Config.TopPos = new Point(left, top);
            }
        }

        public void OnOrderWindowLocked()
        {
            _Config.SaveConfig();
        }
    }
}
