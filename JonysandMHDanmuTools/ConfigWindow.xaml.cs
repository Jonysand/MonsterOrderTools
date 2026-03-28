using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Media;

namespace MonsterOrderWindows
{
    /// <summary>
    /// Interaction logic for ConfigWindow.xaml
    /// </summary>
    public partial class ConfigWindow : Window
    {
        public ConfigWindow()
        {
            InitializeComponent();
        }

        public void FillConfig(MainConfig config)
        {
            if (config == null) return;
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

            // 小米MiMo TTS配置
            MimoSpeedSlider.Value = config.MIMO_SPEED;

            // 设置TTS引擎选择
            switch (config.TTS_ENGINE)
            {
                case "mimo":
                    TTSEngineComboBox.SelectedIndex = 1;
                    break;
                case "sapi":
                    TTSEngineComboBox.SelectedIndex = 2;
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
            // DanmuManager.GetInst().SetMedalName(MedalNameTextBox.Text);
            GlobalEventListener.Invoke("ConfigChanged", "ID_CODE:" + IdentityCodeTextBox.Password);
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
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_RATE:{e.NewValue}");
        }
        private void VoicePitchSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_PITCH:{e.NewValue}");
        }
        private void VoiceVolumeSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            GlobalEventListener.Invoke("ConfigChanged", $"SPEECH_VOLUME:{e.NewValue}");
        }

        private void OnlyMedalCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (OnlyMedalCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_WEARING_MEDAL:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_WEARING_MEDAL:0");
        }

        private void OnlyGuardLevel_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
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
            if (OnlyPaidGiftCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:0");
        }

        private void OpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            GlobalEventListener.Invoke("ConfigChanged", $"OPACITY:{e.NewValue}");
        }

        private void SaveSettingsButton_Click(object sender, RoutedEventArgs e)
        {
            ToolsMain.GetConfigService().SaveConfig();

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
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string engine = selectedItem.Tag?.ToString() ?? "auto";
            GlobalEventListener.Invoke("ConfigChanged", $"TTS_ENGINE:{engine}");

            // 根据引擎选择显示/隐藏配置面板
            if (MimoConfigPanel != null)
            {
                MimoConfigPanel.Visibility = engine == "sapi" ? Visibility.Collapsed : Visibility.Visible;
            }
            if (SapiConfigPanel != null)
            {
                SapiConfigPanel.Visibility = engine == "mimo" ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        private void MimoVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
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
            var comboBox = sender as System.Windows.Controls.ComboBox;
            if (comboBox == null || comboBox.SelectedItem == null)
                return;
            var selectedItem = comboBox.SelectedItem as System.Windows.Controls.ComboBoxItem;
            if (selectedItem == null)
                return;
            string style = selectedItem.Tag?.ToString() ?? "";
            GlobalEventListener.Invoke("ConfigChanged", $"MIMO_STYLE:{style}");
        }

        private void MimoSpeedSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            GlobalEventListener.Invoke("ConfigChanged", $"MIMO_SPEED:{e.NewValue}");
        }
    }
}
