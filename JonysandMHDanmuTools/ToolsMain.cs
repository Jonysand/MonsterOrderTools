﻿using System;
using System.Collections.Generic;
using System.Windows;


namespace MonsterOrderWindows
{
    public class ToolsMain
    {
        private ConfigWindow _ConfigWindow = null;
        private OrderedMonsterWindow _OrderedMonsterWindow = null;
        private ConfigService _Config = null;
        /*
            this.PluginAuth = "鬼酒时雨;Hey_Coder";
            this.PluginName = "点怪姬";
            this.PluginVer = "v0.73";
            this.PluginDesc = "弹幕姬插件开发学习中，祝你每天吃饱饱！";
            this.PluginCont = "QQ: 1600402178";
         */

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
            GlobalEventListener.AddListener("LOG", (object msg) => SendCommand("LOG:" + msg.ToString()));
            GlobalEventListener.AddListener("ConfigChanged", (object msg) => ConfigChanged(msg));
            GlobalEventListener.AddListener("OrderWindowLocked", (object msg) => OnOrderWindowLocked());
            GlobalEventListener.AddListener("Message", (object msg) => OnOrderWindowLocked());
        }

        public void Stop()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Hide();
                }));
            }
        }

        public void Start()
        {
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

        // New interface from MonsterOrderWilds ------------------------------------------------------------------------------------------
        public void OpenConfigWindow()
        {
            if (_ConfigWindow == null)
                _ConfigWindow = new ConfigWindow();
            _ConfigWindow.Show();
        }

        public void OnConnected()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.Show();
                }));
            }
            if (_ConfigWindow != null)
            {
                _ConfigWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _ConfigWindow.SetStatus(true);
                }));
            }
        }

        public void OnDisconnected()
        {
            if (_ConfigWindow != null)
            {
                _ConfigWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _ConfigWindow.SetStatus(false);
                }));
            }
        }

        public void OnHotKeyLock()
        {
            if (_OrderedMonsterWindow != null)
            {
                _OrderedMonsterWindow.Dispatcher.Invoke(new Action(delegate
                {
                    _OrderedMonsterWindow.OnHotKeyLock();
                }));
            }
        }

        // 先保留用CSharp的逻辑处理，后面挪到cpp
        public void OnReceivedRawMsg(String rawJsonStr)
        {
            DanmuManager.GetInst().OnReceicedRawJson(rawJsonStr);
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
    }
}
