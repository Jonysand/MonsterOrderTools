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
            ToolsMain.SendCommand("ConfirmIDCode:" + IdentityCodeTextBox.Password);
        }

        private void IdentityCodeTextBox_PasswordChanged(object sender, RoutedEventArgs e)
        {
            IdentityCodePlaceholder.Visibility =
                string.IsNullOrEmpty(IdentityCodeTextBox.Password) ? Visibility.Visible : Visibility.Collapsed;
        }

        private void UpdateButton_Click(object sender, RoutedEventArgs e)
        {
            ToolsMain.SendCommand("Update:0");
        }

        private void UpdateListButton_Click(object sender, RoutedEventArgs e)
        {
            ToolsMain.SendCommand("UpdateList:0");
        }
    }
}
