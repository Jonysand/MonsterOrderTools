using System;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace MonsterOrderWindows
{
    /// <summary>
    /// Interaction logic for ConfigWindow.xaml
    /// </summary>
    public partial class ConfigWindow : Window
    {
        private bool _isInitializing = true;

        public ConfigWindow()
        {
            InitializeComponent();
            Loaded += (s, e) => _isInitializing = false;
        }

        public void FillConfig(MainConfig config)
        {
            if (config == null) return;

            // 同步到ConfigProxy
            ConfigProxy.Instance.RefreshFromConfig(config);

            IdentityCodeTextBox.Password = config.ID_CODE;
            if (config.ENABLE_VOICE)
                EnableVoiceCheckBox.IsChecked = true;
            if (config.ONLY_MEDAL_ORDER)
                OnlyMedalOrderCheckBox.IsChecked = true;
            VoiceRateSlider.Value = config.SPEECH_RATE;
            VoicePitchSlider.Value = config.SPEECH_PITCH;
            VoiceVolumeSlider.Value = config.SPEECH_VOLUME;
            if (config.ONLY_SPEEK_WEARING_MEDAL)
                OnlyMedalCheckBox.IsChecked = true;
            switch (config.ONLY_SPEEK_GUARD_LEVEL)
            {
                case 0:
                    OnlyGuardLevel.SelectedIndex = 0; // "所有人"
                    break;
                case 3:
                    OnlyGuardLevel.SelectedIndex = 1; // "舰长"
                    break;
                case 2:
                    OnlyGuardLevel.SelectedIndex = 2; // "提督"
                    break;
                case 1:
                    OnlyGuardLevel.SelectedIndex = 3; // "总督"
                    break;
            }
            if (config.ONLY_SPEEK_PAID_GIFT)
                OnlyPaidGiftCheckBox.IsChecked = true;
            OpacitySlider.Value = config.OPACITY;
            var marqueeText = config.DEFAULT_MARQUEE_TEXT;
            DefaultMarqueeTextBox.Text = string.IsNullOrEmpty(marqueeText) ? "发送'点怪 xxx'进行点怪" : marqueeText;

            TtsCacheDaysToKeepTextBox.Text = config.TTS_CACHE_DAYS_TO_KEEP.ToString();

            // 舰长打卡AI配置
            EnableCaptainCheckinAICheckBox.IsChecked = config.ENABLE_CAPTAIN_CHECKIN_AI;
            CheckinTriggerWordsTextBox.Text = config.CHECKIN_TRIGGER_WORDS ?? "打卡,签到";

            // 小米MiMo TTS配置

            // 设置TTS引擎选择
            switch (config.TTS_ENGINE)
            {
                case "minimax":
                    TTSEngineComboBox.SelectedIndex = 1;
                    break;
                case "mimo":
                    TTSEngineComboBox.SelectedIndex = 2;
                    break;
                case "sapi":
                    TTSEngineComboBox.SelectedIndex = 3;
                    break;
                default:
                    TTSEngineComboBox.SelectedIndex = 0; // auto
                    break;
            }

            // 设置语音角色（使用Tag属性）
            for (int i = 0; i < MimoVoiceComboBox.Items.Count; i++)
            {
                var item = MimoVoiceComboBox.Items[i] as System.Windows.Controls.ComboBoxItem;
                if (item != null && item.Tag?.ToString() == config.MIMO_VOICE)
                {
                    MimoVoiceComboBox.SelectedIndex = i;
                    break;
                }
            }

            // 设置语音风格（使用Tag属性）
            for (int i = 0; i < MimoStyleComboBox.Items.Count; i++)
            {
                var item = MimoStyleComboBox.Items[i] as System.Windows.Controls.ComboBoxItem;
                if (item != null && item.Tag?.ToString() == config.MIMO_STYLE)
                {
                    MimoStyleComboBox.SelectedIndex = i;
                    break;
                }
            }

            // 设置 MiniMax 音色（使用Tag属性）
            for (int i = 0; i < MiniMaxVoiceComboBox.Items.Count; i++)
            {
                var item = MiniMaxVoiceComboBox.Items[i] as System.Windows.Controls.ComboBoxItem;
                if (item != null && item.Tag?.ToString() == config.MINIMAX_VOICE_ID)
                {
                    MiniMaxVoiceComboBox.SelectedIndex = i;
                    break;
                }
            }

            // 设置 MiniMax 语速
            MiniMaxSpeedSlider.Value = config.MINIMAX_SPEED;
        }

        public void SetStatus(ConnectionState state, DisconnectReason reason)
        {
            switch (state)
            {
                case ConnectionState.Connected:
                    StatusBG.Background = new SolidColorBrush(Color.FromRgb(71, 219, 155));
                    Status.Content = "已连接";
                    ConnectButton.Content = "断开";
                    break;
                case ConnectionState.Connecting:
                    StatusBG.Background = new SolidColorBrush(Color.FromRgb(255, 253, 231));
                    Status.Content = "连接中...";
                    ConnectButton.Content = "取消";
                    break;
                case ConnectionState.Reconnecting:
                    StatusBG.Background = new SolidColorBrush(Color.FromRgb(255, 253, 231));
                    Status.Content = "正在重连...";
                    ConnectButton.Content = "取消";
                    break;
                case ConnectionState.ReconnectFailed:
                    StatusBG.Background = new SolidColorBrush(Color.FromRgb(255, 183, 183));
                    Status.Content = $"重连失败，原因: {GetReasonText(reason)}";
                    ConnectButton.Content = "连接";
                    break;
                case ConnectionState.Disconnected:
                default:
                    StatusBG.Background = new SolidColorBrush(Color.FromRgb(255, 253, 231));
                    Status.Content = "未连接";
                    ConnectButton.Content = "连接";
                    break;
            }
        }

        private string GetReasonText(DisconnectReason reason)
        {
            switch (reason)
            {
                case DisconnectReason.NetworkError:
                    return "网络错误";
                case DisconnectReason.HeartbeatTimeout:
                    return "心跳超时";
                case DisconnectReason.ServerClose:
                    return "服务器断开";
                case DisconnectReason.AuthFailed:
                    return "鉴权失败";
                default:
                    return "未知";
            }
        }

        public void SetVersion(int version)
        {
            this.Title = $"Monster Order - Wilds (v {version})";
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            e.Cancel = true;
            ToolsMain.SendCommand("Exit:0");
        }

        private void OnConfirmIDCode(object sender, RoutedEventArgs e)
        {
            ToolsMain.SendCommand("ConfirmIDCode:" + IdentityCodeTextBox.Password);
        }

        private void OnConnectButtonClick(object sender, RoutedEventArgs e)
        {
            string buttonText = ConnectButton.Content?.ToString();
            if (buttonText == "断开")
            {
                ToolsMain.SendCommand("Disconnect:");
            }
            else if (buttonText == "连接")
            {
                if (!string.IsNullOrEmpty(IdentityCodeTextBox.Password))
                {
                    GlobalEventListener.Invoke("ConfigChanged", "ID_CODE:" + IdentityCodeTextBox.Password);
                    ToolsMain.SendCommand("ConfirmIDCode:" + IdentityCodeTextBox.Password);
                }
                else
                {
                    ToolsMain.SendCommand("Reconnect:");
                }
            }
            else if (buttonText == "取消")
            {
                ToolsMain.SendCommand("Disconnect:");
            }
        }

        private void IdentityCodeTextBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            IdentityCodePlaceholder.Visibility =
                string.IsNullOrEmpty(IdentityCodeTextBox.Password) ? Visibility.Visible : Visibility.Collapsed;
        }

        private void OnlyMedalOrderCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (OnlyMedalOrderCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_MEDAL_ORDER:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_MEDAL_ORDER:0");
        }

        private void EnableVoiceCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (EnableVoiceCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_VOICE:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_VOICE:0");
        }

        private void VoiceRateSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_RATE:{e.NewValue}");
        }
        private void VoicePitchSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_PITCH:{e.NewValue}");
        }
        private void VoiceVolumeSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_VOLUME:{e.NewValue}");
        }

        private void OnlyMedalCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            if (OnlyMedalCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_WEARING_MEDAL:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_WEARING_MEDAL:0");
        }

        private void OnlyGuardLevel_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string guardLevelText = selectedItem.Content.ToString();
            int guardLevelValue = 0;
            switch (guardLevelText)
            {
                case "所有人":
                    guardLevelValue = 0;
                    break;
                case "舰长":
                    guardLevelValue = 3;
                    break;
                case "提督":
                    guardLevelValue = 2;
                    break;
                case "总督":
                    guardLevelValue = 1;
                    break;
            }
            GlobalEventListener.Invoke("ConfigChanged", $"ONLY_SPEEK_GUARD_LEVEL:{guardLevelValue}");
        }

        private void OnlyPaidGiftCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            if (OnlyPaidGiftCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:0");
        }

        private void OpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"OPACITY:{e.NewValue}");
        }

        private void PenetratingModeOpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"PENETRATING_MODE_OPACITY:{e.NewValue}");
        }

        private async void SaveSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            SaveSettingsButton.IsEnabled = false;
            SaveSettingsButton.Content = "保存中...";

            await Task.Run(() =>
            {
                ToolsMain.GetConfigService().SaveConfig();
            });

            SaveSettingsButton.Content = "保存设置";
            SaveSettingsButton.IsEnabled = true;

            var tip = new System.Windows.Controls.ToolTip
            {
                Content = "保存设置成功",
                PlacementTarget = SaveSettingsButton,
                Placement = System.Windows.Controls.Primitives.PlacementMode.Top,
                StaysOpen = false,
                IsOpen = true
            };
            var timer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1.5)
            };
            timer.Tick += (s, args) =>
            {
                tip.IsOpen = false;
                timer.Stop();
            };
            timer.Start();
        }

        private void TTSEngineComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string engine = selectedItem.Tag?.ToString() ?? "auto";
            GlobalEventListener.Invoke("ConfigChanged", $"TTS_ENGINE:{engine}");
        }

        private void MimoVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string voice = selectedItem.Tag?.ToString() ?? "mimo_default";
            GlobalEventListener.Invoke("ConfigChanged", $"MIMO_VOICE:{voice}");
        }

        private void MimoStyleComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string style = selectedItem.Tag?.ToString() ?? "";
            GlobalEventListener.Invoke("ConfigChanged", $"MIMO_STYLE:{style}");
        }

        private void MiniMaxVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string voiceId = selectedItem.Tag?.ToString() ?? "female-tianmei";
            GlobalEventListener.Invoke("ConfigChanged", $"MINIMAX_VOICE_ID:{voiceId}");
        }

        private void MiniMaxSpeedSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", $"MINIMAX_SPEED:{e.NewValue}");
        }

        private void LockWindowButton_Click(object sender, RoutedEventArgs e)
        {
            GlobalEventListener.Invoke("OrderWindowLocked", null);
        }

        public void OnLockStateChanged(object sender, bool isLocked)
        {
            Dispatcher.Invoke(() =>
            {
                LockWindowButton.Content = isLocked ? "解锁窗口" : "锁定窗口";
            });
        }

        public void InitLockButtonState(bool isLocked)
        {
            LockWindowButton.Content = isLocked ? "解锁窗口" : "锁定窗口";
        }

        private void DefaultMarqueeTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            GlobalEventListener.Invoke("ConfigChanged", "DEFAULT_MARQUEE_TEXT:" + DefaultMarqueeTextBox.Text);
        }

        private void TtsCacheDaysToKeepTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (TtsCacheDaysToKeepTextBox == null) return;
            
            if (int.TryParse(TtsCacheDaysToKeepTextBox.Text, out int days))
            {
                if (days < 1) days = 1;
                if (days > 365) days = 365;
                
                if (ConfigProxy.Instance.TtsCacheDaysToKeep != days)
                {
                    ConfigProxy.Instance.TtsCacheDaysToKeep = days;
                    GlobalEventListener.Invoke("ConfigChanged", "TTS_CACHE_DAYS_TO_KEEP:" + days);
                }
            }
        }

        private void EnableCaptainCheckinAICheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (EnableCaptainCheckinAICheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_CAPTAIN_CHECKIN_AI:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_CAPTAIN_CHECKIN_AI:0");
        }

        private void CheckinTriggerWordsTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            GlobalEventListener.Invoke("ConfigChanged", "CHECKIN_TRIGGER_WORDS:" + CheckinTriggerWordsTextBox.Text);
        }
    }
}
