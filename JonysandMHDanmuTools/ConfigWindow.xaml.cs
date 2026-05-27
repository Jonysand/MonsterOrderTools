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
        private System.Windows.Threading.DispatcherTimer _ttsEngineUpdateTimer;

        public ConfigWindow()
        {
            InitializeComponent();

            if (ToolsMain.IsOnlyOrderMonster)
            {
                TabVoiceSettings.Visibility = Visibility.Collapsed;
                TabCaptainCheckin.Visibility = Visibility.Collapsed;
            }

            Loaded += (s, e) =>
            {
                _isInitializing = false;
                if (!ToolsMain.IsOnlyOrderMonster)
                    StartTTSEngineUpdateTimer();
            };
            Closed += (s, e) =>
            {
                _ttsEngineUpdateTimer?.Stop();
            };
        }

        private void StartTTSEngineUpdateTimer()
        {
            _ttsEngineUpdateTimer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(2)
            };
            _ttsEngineUpdateTimer.Tick += (s, e) =>
            {
                UpdateCurrentTTSEngineLabel();
            };
            _ttsEngineUpdateTimer.Start();
        }

        public void FillConfig(MainConfig config)
        {
            if (config == null) return;

            // 同步到ConfigProxy
            ConfigProxy.Instance.RefreshFromConfig(config);

            IdentityCodeTextBox.Password = config.ID_CODE;
            ManboApiKeyTextBox.Password = config.MANBO_API_KEY ?? "";
            ManboApiKeyPlaceholder.Visibility =
                string.IsNullOrEmpty(ManboApiKeyTextBox.Password) ? Visibility.Visible : Visibility.Collapsed;

            // 填充音色列表
            ManboVoiceComboBox.Items.Clear();
            ManboVoiceComboBox.Items.Add(new System.Windows.Controls.ComboBoxItem 
            { 
                Content = "曼波（付费）", 
                Tag = "曼波" 
            });
            
            foreach (var voice in ToolsMain.ManboVoiceList)
            {
                if (voice != "曼波")
                {
                    ManboVoiceComboBox.Items.Add(new System.Windows.Controls.ComboBoxItem 
                    { 
                        Content = voice, 
                        Tag = voice 
                    });
                }
            }
            
            // 选择保存的音色
            string savedVoice = config.MANBO_VOICE ?? "曼波";
            bool found = false;
            foreach (System.Windows.Controls.ComboBoxItem item in ManboVoiceComboBox.Items)
            {
                if ((string)item.Tag == savedVoice)
                {
                    ManboVoiceComboBox.SelectedItem = item;
                    found = true;
                    break;
                }
            }
            if (!found && !string.IsNullOrEmpty(savedVoice))
            {
                // 如果保存的音色不在列表中，添加并选中
                var newItem = new System.Windows.Controls.ComboBoxItem 
                { 
                    Content = savedVoice, 
                    Tag = savedVoice 
                };
                ManboVoiceComboBox.Items.Add(newItem);
                ManboVoiceComboBox.SelectedItem = newItem;
            }

            if (config.ONLY_MEDAL_ORDER)
                OnlyMedalOrderCheckBox.IsChecked = true;

            if (!ToolsMain.IsOnlyOrderMonster)
            {
            if (config.ENABLE_VOICE)
                EnableVoiceCheckBox.IsChecked = true;
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
            }
            OpacitySlider.Value = config.OPACITY;
            var marqueeText = config.DEFAULT_MARQUEE_TEXT;
            DefaultMarqueeTextBox.Text = string.IsNullOrEmpty(marqueeText) ? "发送'点怪 xxx'进行点怪" : marqueeText;

            if (!ToolsMain.IsOnlyOrderMonster)
            {
            TtsCacheDaysToKeepTextBox.Text = config.TTS_CACHE_DAYS_TO_KEEP.ToString();

            // 舰长打卡AI配置
            EnableCaptainCheckinAICheckBox.IsChecked = config.ENABLE_CAPTAIN_CHECKIN_AI;
            CheckinTriggerWordsTextBox.Text = config.CHECKIN_TRIGGER_WORDS ?? "打卡,签到";

            // 小米MiMo TTS配置

            // 设置TTS引擎选择（顺序：自动/Manbo/小米MiMo/Windows SAPI）
            switch (config.TTS_ENGINE)
            {
                case "manbo":
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

            // 更新当前实际TTS引擎显示
            UpdateCurrentTTSEngineLabel();

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

        private void ManboApiKeyTextBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            ManboApiKeyPlaceholder.Visibility =
                string.IsNullOrEmpty(ManboApiKeyTextBox.Password) ? Visibility.Visible : Visibility.Collapsed;
            GlobalEventListener.Invoke("ConfigChanged", "MANBO_API_KEY:" + ManboApiKeyTextBox.Password);
        }

        private void ManboVoiceComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (_isInitializing) return;
            if (ManboVoiceComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem item)
            {
                string voice = item.Tag as string;
                if (!string.IsNullOrEmpty(voice))
                {
                    GlobalEventListener.Invoke("ConfigChanged", $"MANBO_VOICE:{voice}");
                }
            }
        }

        private void OnlyMedalOrderCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            if (OnlyMedalOrderCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_MEDAL_ORDER:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_MEDAL_ORDER:0");
        }

        private void EnableVoiceCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
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
            try
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
            catch (Exception ex)
            {
                SaveSettingsButton.Content = "保存设置";
                SaveSettingsButton.IsEnabled = true;
                System.Diagnostics.Debug.WriteLine($"[SaveSettingsButton_Click] Exception: {ex.Message}");
            }
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

            // 更新当前实际TTS引擎显示
            UpdateCurrentTTSEngineLabel();
        }

        private void UpdateCurrentTTSEngineLabel()
        {
            try
            {
                var sb = new System.Text.StringBuilder(64);
                NativeImports.TTSManager_GetCurrentProviderName(sb, 64);
                string providerName = sb.ToString().TrimEnd('\0');

                string displayName = providerName switch
                {
                    "manbo" => "Manbo",
                    "xiaomi" => "小米MiMo",
                    "sapi" => "Windows SAPI",
                    "" => "未知",
                    _ => providerName
                };

                if (CurrentTTSEngineLabel != null)
                {
                    CurrentTTSEngineLabel.Content = $"当前引擎: {displayName}";
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"UpdateCurrentTTSEngineLabel failed: {ex.Message}");
                if (CurrentTTSEngineLabel != null)
                    CurrentTTSEngineLabel.Content = "当前引擎: 获取失败";
            }
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
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", "DEFAULT_MARQUEE_TEXT:" + DefaultMarqueeTextBox.Text);
        }

        private void TtsCacheDaysToKeepTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (_isInitializing) return;
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
            if (_isInitializing) return;
            if (EnableCaptainCheckinAICheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_CAPTAIN_CHECKIN_AI:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ENABLE_CAPTAIN_CHECKIN_AI:0");
        }

        private void CheckinTriggerWordsTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (_isInitializing) return;
            GlobalEventListener.Invoke("ConfigChanged", "CHECKIN_TRIGGER_WORDS:" + CheckinTriggerWordsTextBox.Text);
        }

        private void BatchCheckinButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var messageBytes = new byte[4096];
                bool success = NativeImports.ProfileManager_BatchCheckin(messageBytes, messageBytes.Length);
                string message = System.Text.Encoding.UTF8.GetString(messageBytes).TrimEnd('\0');

                if (success)
                {
                    MessageBox.Show(message, "一键黑幕执行成功", MessageBoxButton.OK, MessageBoxImage.Information);
                }
                else
                {
                    MessageBox.Show(message, "一键黑幕执行失败", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("执行一键黑幕时出错：" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void ExportCheckinRecordsButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // 显示导出选项对话框
                var dialog = new ExportOptionsDialog();
                if (dialog.ShowDialog() == true)
                {
                    // 获取用户选择的选项
                    string format = dialog.SelectedFormat;
                    string username = dialog.SelectedUsername;
                    int startDate = dialog.StartDate;
                    int endDate = dialog.EndDate;

                    // 根据是否填写用户名决定默认文件名
                    string defaultFileName = string.IsNullOrEmpty(username)
                        ? $"users_summary_{DateTime.Now:yyyyMMdd}"
                        : $"checkin_records_{username}_{DateTime.Now:yyyyMMdd}";

                    // 显示文件保存对话框
                    var saveDialog = new Microsoft.Win32.SaveFileDialog();
                    saveDialog.Filter = format == "csv" ? "CSV文件 (*.csv)|*.csv" : "JSON文件 (*.json)|*.json";
                    saveDialog.DefaultExt = format;
                    saveDialog.FileName = defaultFileName;

                    if (saveDialog.ShowDialog() == true)
                    {
                        string filePath = saveDialog.FileName;
                        var messageBytes = new byte[4096];
                        bool success;

                        if (string.IsNullOrEmpty(username))
                        {
                            // 没有填写用户名，导出所有用户汇总数据
                            success = NativeImports.ProfileManager_ExportUsersSummary(
                                filePath, format, messageBytes, messageBytes.Length);
                        }
                        else
                        {
                            // 填写了用户名，导出该用户的详细打卡记录
                            success = NativeImports.ProfileManager_ExportCheckinRecords(
                                filePath, format, username, startDate, endDate, messageBytes, messageBytes.Length);
                        }

                        string message = System.Text.Encoding.UTF8.GetString(messageBytes).TrimEnd('\0');

                        if (success)
                        {
                            MessageBox.Show($"导出成功！{message}", "导出完成", MessageBoxButton.OK, MessageBoxImage.Information);
                        }
                        else
                        {
                            MessageBox.Show($"导出失败：{message}", "导出错误", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("导出打卡记录时出错：" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
