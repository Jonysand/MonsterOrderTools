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
    public class ToolsMain : BilibiliDM_PluginFramework.DMPlugin
    {
        private ConfigWindow _ConfigWindow = null;
        private OrderedMonsterWindow _OrderedMonsterWindow = null;
        private DanmuManager _DanmuManager = null;

        public ToolsMain()
        {
            this.Connected += OnConnected;
            this.Disconnected += OnDisconnected;
            this.ReceivedDanmaku += OnReceivedDanmaku;
            this.ReceivedRoomCount += OnReceivedRoomCount;
            this.PluginAuth = "鬼酒时雨;Hey_Coder";
            this.PluginName = "点怪姬";
            this.PluginVer = "v0.2";
            this.PluginDesc = "弹幕姬插件开发学习中，祝你每天吃饱饱！";
            this.PluginCont = "QQ: 1600402178";
        }

        private void OnReceivedRoomCount(object sender, BilibiliDM_PluginFramework.ReceivedRoomCountArgs e)
        {
            
        }

        private void OnDisconnected(object sender, BilibiliDM_PluginFramework.DisconnectEvtArgs e)
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Hide();
                    _OrderedMonsterWindow.Topmost = false;
                }));
            }
        }

        private void OnReceivedDanmaku(object sender, BilibiliDM_PluginFramework.ReceivedDanmakuArgs e)
        {
            _DanmuManager.OnReceivedDanmaku(sender, e);
        }

        public override void Inited()
        {
            try
            {
                _OrderedMonsterWindow = new OrderedMonsterWindow(this);
            }
            catch (Exception e)
            {
                MessageBox.Show($"点怪窗口启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
            }

            try
            {
                _DanmuManager = new DanmuManager(this);
                _DanmuManager.SetOrderedMonsterWindow(_OrderedMonsterWindow);
            }
            catch (Exception e)
            {
                MessageBox.Show($"启动失败,请将桌面上的错误报告发送给作者（/TДT)/\n{e}", "零食小插件", 0, MessageBoxImage.Error);
                throw;
            }
        }

        private void OnConnected(object sender, BilibiliDM_PluginFramework.ConnectedEvtArgs e)
        {
            if (_OrderedMonsterWindow != null && this.Status)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                    _OrderedMonsterWindow.Topmost = true;
                }));
            }
        }

        public override void Admin()
        {
            base.Admin();

            if (_ConfigWindow == null)
                _ConfigWindow = new ConfigWindow();
            _ConfigWindow.Show();
            _ConfigWindow.Topmost = true;
            _ConfigWindow.Topmost = false;
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
                    _OrderedMonsterWindow.Topmost = false;
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
                    _OrderedMonsterWindow.Topmost = true;
                }));
            }
        }

        public void RemoveRecord()
        {
            _DanmuManager.RemoveRecord();
        }
    }
}
