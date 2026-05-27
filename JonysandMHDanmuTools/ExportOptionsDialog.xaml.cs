using System;
using System.Windows;

namespace MonsterOrderWindows
{
    public partial class ExportOptionsDialog : Window
    {
        public string SelectedFormat { get; private set; } = "csv";
        public string SelectedUsername { get; private set; } = "";
        public int StartDate { get; private set; } = 0;
        public int EndDate { get; private set; } = 0;

        public ExportOptionsDialog()
        {
            InitializeComponent();
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            // 获取格式
            if (FormatComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem formatItem)
            {
                SelectedFormat = formatItem.Content.ToString().ToLower();
            }

            // 获取用户名
            SelectedUsername = UsernameTextBox.Text?.Trim() ?? "";

            // 获取日期范围
            if (StartDatePicker.SelectedDate.HasValue)
            {
                var date = StartDatePicker.SelectedDate.Value;
                StartDate = date.Year * 10000 + date.Month * 100 + date.Day;
            }

            if (EndDatePicker.SelectedDate.HasValue)
            {
                var date = EndDatePicker.SelectedDate.Value;
                EndDate = date.Year * 10000 + date.Month * 100 + date.Day;
            }

            // 验证日期范围
            if (StartDate > 0 && EndDate > 0 && StartDate > EndDate)
            {
                MessageBox.Show("开始日期不能大于结束日期", "日期错误", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            DialogResult = true;
            Close();
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }
    }
}