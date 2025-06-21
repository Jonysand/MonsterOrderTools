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
                OnlyPaidGitfCheckBox.IsChecked = true;
        }

        public void SetStatus(bool connected)
        {
            if (connected)
            {
                StatusBG.Background = new SolidColorBrush(Color.FromRgb(71, 219, 155));
                Status.Content = "已连接";
            }
            else
            {
                StatusBG.Background = new SolidColorBrush(Color.FromRgb(255, 253, 231));
                Status.Content = "未连接";
            }
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

        private void IdentityCodeTextBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            IdentityCodePlaceholder.Visibility =
                string.IsNullOrEmpty(IdentityCodeTextBox.Password) ? Visibility.Visible : Visibility.Collapsed;
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

        private void OnlyPaidGitfCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            if (OnlyPaidGitfCheckBox.IsChecked == true)
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:1");
            else
                GlobalEventListener.Invoke("ConfigChanged", "ONLY_SPEEK_PAID_GIFT:0");
        }

        private void UpdateButton_Click(object sender, RoutedEventArgs e)
        {
            ToolsMain.SendCommand("Update:0");
        }

        private void UpdateListButton_Click(object sender, RoutedEventArgs e)
        {
            ToolsMain.SendCommand("UpdateList:0");
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
    }
}
