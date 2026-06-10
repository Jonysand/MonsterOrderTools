using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Input;

namespace MonsterOrderWindows
{
    public partial class GMRetroactiveCardDialog : Window
    {
        private class UserItem
        {
            public string Uid { get; set; }
            public string Username { get; set; }
            public int CardCount { get; set; }

            public override string ToString()
            {
                return $"{Username} (uid: {Uid}, 当前补签卡: {CardCount})";
            }
        }

        private string _selectedUid = null;
        private string _selectedUsername = null;

        public GMRetroactiveCardDialog()
        {
            InitializeComponent();
            SearchKeywordTextBox.Focus();
        }

        private void SearchKeywordTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SearchButton_Click(sender, e);
            }
        }

        private void SearchButton_Click(object sender, RoutedEventArgs e)
        {
            string keyword = SearchKeywordTextBox.Text?.Trim();
            if (string.IsNullOrEmpty(keyword))
            {
                SearchStatusText.Text = "请输入搜索关键词";
                return;
            }

            try
            {
                SearchButton.IsEnabled = false;
                SearchStatusText.Text = "搜索中...";
                UsersListBox.Items.Clear();
                _selectedUid = null;
                _selectedUsername = null;
                SelectedUserText.Text = "未选择用户";

                var jsonBytes = new byte[65536];
                var messageBytes = new byte[4096];
                bool success = NativeImports.ProfileManager_SearchUsers(keyword, jsonBytes, jsonBytes.Length, messageBytes, messageBytes.Length);
                string jsonResult = System.Text.Encoding.UTF8.GetString(jsonBytes).TrimEnd('\0');
                string message = System.Text.Encoding.UTF8.GetString(messageBytes).TrimEnd('\0');

                if (!success && string.IsNullOrEmpty(jsonResult))
                {
                    // 检查是否是缓冲区太小的错误
                    if (message.Contains("buffer too small"))
                        SearchStatusText.Text = "搜索结果过多，请缩小搜索范围";
                    else
                        SearchStatusText.Text = string.IsNullOrEmpty(message) ? "搜索失败" : message;
                    SearchButton.IsEnabled = true;
                    return;
                }

                if (string.IsNullOrEmpty(jsonResult) || jsonResult == "[]")
                {
                    SearchStatusText.Text = "未找到匹配用户";
                    SearchButton.IsEnabled = true;
                    return;
                }

                // 解析JSON
                var users = ParseUserJson(jsonResult);
                foreach (var user in users)
                {
                    UsersListBox.Items.Add(user);
                }

                SearchStatusText.Text = $"找到 {users.Count} 个匹配用户";
            }
            catch (Exception ex)
            {
                SearchStatusText.Text = "搜索出错: " + ex.Message;
                System.Diagnostics.Debug.WriteLine($"[GMRetroactiveCardDialog] Search error: {ex.Message}");
            }
            finally
            {
                SearchButton.IsEnabled = true;
            }
        }

        private List<UserItem> ParseUserJson(string json)
        {
            var result = new List<UserItem>();
            try
            {
                // 简单JSON解析：[{"uid":"xxx","username":"xxx","cardCount":0}, ...]
                if (string.IsNullOrEmpty(json) || json == "[]")
                    return result;

                // 去掉外层 []
                string content = json.Trim();
                if (content.StartsWith("[")) content = content.Substring(1);
                if (content.EndsWith("]")) content = content.Substring(0, content.Length - 1);
                content = content.Trim();
                if (string.IsNullOrEmpty(content))
                    return result;

                // 按 },  分割各个对象
                var objects = new List<string>();
                int depth = 0;
                int start = 0;
                for (int i = 0; i < content.Length; i++)
                {
                    if (content[i] == '{') depth++;
                    else if (content[i] == '}') depth--;
                    if (depth == 0 && content[i] == '}')
                    {
                        objects.Add(content.Substring(start, i - start + 1));
                        // 跳过逗号和空白
                        int next = i + 1;
                        while (next < content.Length && (content[next] == ',' || content[next] == ' ' || content[next] == '\n' || content[next] == '\r'))
                            next++;
                        start = next;
                        i = next - 1;
                    }
                }

                foreach (var obj in objects)
                {
                    var user = new UserItem();
                    user.Uid = ExtractJsonString(obj, "uid");
                    user.Username = ExtractJsonString(obj, "username");
                    user.CardCount = ExtractJsonInt(obj, "cardCount");
                    result.Add(user);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"[GMRetroactiveCardDialog] JSON parse error: {ex.Message}");
            }
            return result;
        }

        private string ExtractJsonString(string jsonObj, string key)
        {
            string search = "\"" + key + "\":\"";
            int startIdx = jsonObj.IndexOf(search);
            if (startIdx < 0) return "";
            startIdx += search.Length;
            // 处理转义引号：找到未转义的结束引号
            int endIdx = startIdx;
            while (endIdx < jsonObj.Length)
            {
                if (jsonObj[endIdx] == '"' && (endIdx == startIdx || jsonObj[endIdx - 1] != '\\'))
                    break;
                endIdx++;
            }
            if (endIdx >= jsonObj.Length) return "";
            return jsonObj.Substring(startIdx, endIdx - startIdx);
        }

        private int ExtractJsonInt(string jsonObj, string key)
        {
            string search = "\"" + key + "\":";
            int startIdx = jsonObj.IndexOf(search);
            if (startIdx < 0) return 0;
            startIdx += search.Length;
            // 跳过空白
            while (startIdx < jsonObj.Length && jsonObj[startIdx] == ' ') startIdx++;
            int endIdx = startIdx;
            while (endIdx < jsonObj.Length && (char.IsDigit(jsonObj[endIdx]) || jsonObj[endIdx] == '-'))
                endIdx++;
            if (endIdx == startIdx) return 0;
            int.TryParse(jsonObj.Substring(startIdx, endIdx - startIdx), out int val);
            return val;
        }

        private void UsersListBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (UsersListBox.SelectedItem is UserItem selectedUser)
            {
                _selectedUid = selectedUser.Uid;
                _selectedUsername = selectedUser.Username;
                SelectedUserText.Text = $"已选择: {selectedUser.Username} (当前补签卡: {selectedUser.CardCount})";
            }
            else
            {
                _selectedUid = null;
                _selectedUsername = null;
                SelectedUserText.Text = "未选择用户";
            }
        }

        private void ConfirmButton_Click(object sender, RoutedEventArgs e)
        {
            // 验证用户选择
            if (string.IsNullOrEmpty(_selectedUid))
            {
                MessageBox.Show("请先选择一个用户", "提示", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // 验证数量
            if (!int.TryParse(CardCountTextBox.Text, out int count) || count <= 0)
            {
                MessageBox.Show("请输入有效的补签卡数量（正整数）", "提示", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // 二次确认
            var confirmResult = MessageBox.Show(
                $"确认为 {_selectedUsername} 发放 {count} 张补签卡？",
                "确认发放",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (confirmResult != MessageBoxResult.Yes)
                return;

            // 执行发放
            try
            {
                ConfirmButton.IsEnabled = false;
                var messageBytes = new byte[4096];
                bool success = NativeImports.ProfileManager_AddRetroactiveCards(_selectedUid, count, messageBytes, messageBytes.Length);
                string message = System.Text.Encoding.UTF8.GetString(messageBytes).TrimEnd('\0');

                if (success)
                {
                    MessageBox.Show($"发放成功！{message}", "成功", MessageBoxButton.OK, MessageBoxImage.Information);
                    // 刷新搜索结果
                    SearchButton_Click(sender, e);
                }
                else
                {
                    MessageBox.Show($"发放失败：{message}", "失败", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("发放补签卡时出错：" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                System.Diagnostics.Debug.WriteLine($"[GMRetroactiveCardDialog] AddRetroactiveCards error: {ex.Message}");
            }
            finally
            {
                ConfirmButton.IsEnabled = true;
            }
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }
}
