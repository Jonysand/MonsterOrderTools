using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Windows.Threading;


namespace MonsterOrderWindows
{
    /// <summary>
    /// Interaction logic for OrderedMonsterWindow.xaml
    /// </summary>
    public partial class OrderedMonsterWindow : Window
    {
        // 拖曳时的画布
        private AdornerLayer mAdornerLayer = null;
        // 显示info的队列
        private Queue<RollingInfo> mInfoQueue;
        private string _defaultInfo = "发送'点怪 xxx'进行点怪";
        // 界面是否锁定
        private bool mIsLocked = false;
        public bool IsLocked => mIsLocked;
        public event EventHandler<bool> LockStateChanged;
        // ObservableCollection for virtualization support
        private ObservableCollection<MonsterOrderInfo> _orderCollection = new ObservableCollection<MonsterOrderInfo>();
        // 节流机制
        private const int THROTTLE_MS = 100;
        private DateTime _lastRefreshTime = DateTime.MinValue;
        // 跑马灯状态管理
        private bool _isShowingDefault = true;
        private bool _currentAnimationIsLoop = true;
        // 气泡管理
        private List<AIBubbleControl> _bubbles = new List<AIBubbleControl>();
        private List<DispatcherTimer> _bubbleTimers = new List<DispatcherTimer>();
        private const int MAX_BUBBLES = 5;
        private const int BUBBLE_INTERVAL_MS = 8000;
        private const double BUBBLE_MARGIN = 8;

        public OrderedMonsterWindow()
        {
            InitializeComponent();

            _defaultInfo = ToolsMain.GetConfigService().Config.DEFAULT_MARQUEE_TEXT;
            if (string.IsNullOrEmpty(_defaultInfo))
                _defaultInfo = "发送'点怪 xxx'进行点怪";

            InfoText_Animation.Duration = new Duration(new TimeSpan(0, 0, 10));
            InfoText.Text = _defaultInfo;
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;

            GlobalEventListener.AddListener("AddRollingInfo", (object rollingInfo) => AddRollingInfo(rollingInfo as RollingInfo));
            GlobalEventListener.AddListener("RefreshOrder", (object _) => RefreshOrder());
            GlobalEventListener.AddListener("MarqueeTextChanged", (object text) => UpdateMarqueeText(text?.ToString() ?? ""));
            GlobalEventListener.AddListener("AIReplyBubble", (object info) => AddBubble(info as AIBubbleInfo));

            _isShowingDefault = true;
            _currentAnimationIsLoop = true;
            StartMarqueeAnimation(true);

            // 注册事件通知
            EventDispatcher.Instance.OnQueueChanged += () =>
            {
                var now = DateTime.Now;
                if ((now - _lastRefreshTime).TotalMilliseconds < THROTTLE_MS)
                    return;
                _lastRefreshTime = now;
                Dispatcher.InvokeAsync(new Action(() => RefreshOrder()));
            };

            this.Closing += (s, e) => CleanupBubbles();
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            this.Height = 360;
            this.Width = 440;

            mInfoQueue = new Queue<RollingInfo>();

            MainList.ItemsSource = _orderCollection;
            RefreshWindow();
            RefreshOrder();
            // Hotkey.Regist(this, HotkeyModifiers.Alt, Key.Decimal, OnHotKeyLock);
        }

        public void RefreshWindow()
        {
            float opacity = mIsLocked 
                ? ToolsMain.GetConfigService().Config.PENETRATING_MODE_OPACITY / 100f
                : ToolsMain.GetConfigService().Config.OPACITY / 100f;
            int alphaInt = (int)(255f * opacity + 0.5f);
            alphaInt = Math.Max(0, Math.Min(255, alphaInt));
            byte alpha = (byte)alphaInt;
            var grayBrush = new SolidColorBrush(Color.FromArgb(alpha, 0, 0, 0));
            grayBrush.Freeze();
            MainGrid.Background = grayBrush;

            if (mIsLocked)
            {
                // 背景透明，且可以穿透
                //MainWindow.Background = Brushes.Transparent;
                var bgBrush = new SolidColorBrush(Color.FromArgb(alpha, 0xff, 0xff, 0xff));
                bgBrush.Freeze();
                MainWindow.Background = bgBrush;

                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                uint extendedStyle = GetWindowLong(hwnd, -20);
                SetWindowLong(hwnd, -20, extendedStyle | 0x20u);
                Topmost = true;
                if (MainList.Items.Count > 0)
                    MainList.ScrollIntoView(MainList.Items[0]);
            }
            else
            {
                // 背景调黑，且可以操作窗口
                //MainWindow.Background = Brushes.Gray;
                var bgBrush = new SolidColorBrush(Color.FromArgb(alpha, 0x80, 0x80, 0x80));
                bgBrush.Freeze();
                MainWindow.Background = bgBrush;

                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                uint extendedStyle = GetWindowLong(hwnd, -20);
                SetWindowLong(hwnd, -20, extendedStyle & ~0x20u);
                Topmost = false;
            }
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            e.Cancel = true;
            Hide();
        }

        // 热键锁定窗口
        [DllImport("user32", EntryPoint = "SetWindowLong")]
        private static extern uint SetWindowLong(IntPtr hwnd, int nIndex, uint dwNewLong);
        [DllImport("user32", EntryPoint = "GetWindowLong")]
        private static extern uint GetWindowLong(IntPtr hwnd, int nIndex);
        public void OnHotKeyLock()
        {
            mIsLocked = !mIsLocked;
            RefreshWindow();
            LockStateChanged?.Invoke(this, mIsLocked);
        }

        // 跑马灯动画控制
        private void StartMarqueeAnimation(bool loop)
        {
            MarqueeStoryboard.Stop();
            InfoText_Animation.RepeatBehavior = loop ? RepeatBehavior.Forever : new RepeatBehavior(1);
            _currentAnimationIsLoop = loop;
            MarqueeStoryboard.Begin();
        }

        private void OnMarqueeAnimationCompleted(object sender, EventArgs e)
        {
            if (mInfoQueue.Count > 0)
            {
                var rollingInfo = mInfoQueue.Dequeue();
                InfoText.Text = rollingInfo.Text;
                InfoText.Foreground = new SolidColorBrush(rollingInfo.TextColor);
                InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
                _isShowingDefault = false;
                StartMarqueeAnimation(false);
            }
            else
            {
                InfoText.Text = _defaultInfo;
                InfoText.Foreground = new SolidColorBrush(Colors.LightYellow);
                InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
                _isShowingDefault = true;
                StartMarqueeAnimation(true);
            }
        }

        private void UpdateMarqueeText(string text)
        {
            if (string.IsNullOrEmpty(text))
                text = "发送'点怪 xxx'进行点怪";
            _defaultInfo = text;
            if (_isShowingDefault)
            {
                InfoText.Text = _defaultInfo;
                InfoText.Foreground = new SolidColorBrush(Colors.LightYellow);
                InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
            }
        }

        // 左键窗口任意位置拖曳
        [DllImport("user32.dll")]
        public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        private void OnClientAreaMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (mIsLocked)
                return;
            if (e.ChangedButton == MouseButton.Left)
            {
                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                SendMessage(hwnd, 0x112, (IntPtr)0xF012, IntPtr.Zero);

                GlobalEventListener.Invoke("ConfigChanged", "WindowPosition");
            }
        }

        // 点击以完成订单
        private async void OnClickOrder(object sender, MouseButtonEventArgs e)
        {
            if (mIsLocked)
                return;
            if (e.ChangedButton != MouseButton.Left)
                return;
            Point pos = e.GetPosition(MainList);
            HitTestResult result = VisualTreeHelper.HitTest(MainList, pos);
            if (result == null)
                return;
            ListViewItem selectedItem = Utils.FindVisualParent<ListViewItem>(result.VisualHit);
            if (selectedItem == null || selectedItem.DataContext == null)
                return;
            MonsterOrderInfo orderInfo = selectedItem.DataContext as MonsterOrderInfo;
            int index = _orderCollection.IndexOf(orderInfo);
            if (index >= 0)
            {
                PriorityQueue.GetInst().Dequeue(index);
                await Dispatcher.InvokeAsync(() => RefreshOrder());
            }
        }

        // 添加跑马灯消息
        public void AddRollingInfo(RollingInfo rollingInfo)
        {
            if (_isShowingDefault && mInfoQueue.Count == 0)
            {
                InfoText.Text = rollingInfo.Text;
                InfoText.Foreground = new SolidColorBrush(rollingInfo.TextColor);
                InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
                _isShowingDefault = false;
                StartMarqueeAnimation(false);
            }
            else
            {
                mInfoQueue.Enqueue(rollingInfo);
            }
        }

        public async void RefreshOrder()
        {
            ToolsMain.SendCommand("Log:RefreshOrder called");
            // 后台排序 + 数据准备，然后在UI线程更新列表
            List<MonsterOrderInfo> sortedItems = null;
            try
            {
                sortedItems = await Task.Run(() =>
                {
                PriorityQueue.GetInst().SortQueue();

                // 同步到PriorityQueueProxy
                PriorityQueueProxy.Instance.RefreshFromQueue(PriorityQueue.GetInst());

                var items = new List<MonsterOrderInfo>();
                foreach (var node in PriorityQueue.GetInst().Queue)
                {
                    var tempData = new MonsterOrderInfo();
                    tempData.AudienceName = node.UserName;
                    tempData.MonsterName = node.MonsterName;
                    tempData.GuardLevel = node.GuardLevel;
                    tempData.TemperedLevel = node.TemperedLevel;
                    string iconPath = MonsterData.GetInst().GetMatchedMonsterIconUrl(tempData.MonsterName);
                    if (!string.IsNullOrEmpty(iconPath))
                    {
                        tempData.MonsterIcon = MonsterIconLoader.LoadIcon(iconPath);
                        if (tempData.MonsterIcon == null)
                        {
                            ToolsMain.SendCommand("Log:Failed to load icon for " + tempData.MonsterName + " path=" + iconPath);
                        }
                    }
                    else
                    {
                        ToolsMain.SendCommand("Log:Empty icon path for monster=" + tempData.MonsterName);
                    }
                    items.Add(tempData);
                }
                return items;
            });

            await Dispatcher.InvokeAsync(() =>
            {
                _orderCollection.Clear();
                foreach (var item in sortedItems)
                {
                    _orderCollection.Add(item);
                }
                PriorityQueue.GetInst().SaveList();
            });
            }
            catch (Exception ex)
            {
                ToolsMain.SendCommand("Log:RefreshOrder exception: " + ex.Message);
            }
        }

        // 拖拽排序 -------------------------------------
        private void MainList_QueryContinueDrag(object sender, QueryContinueDragEventArgs e)
        {
            if (mIsLocked)
                return;
            if (mAdornerLayer == null)
                return;
            mAdornerLayer.Update();
        }
        private void MainList_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (mIsLocked)
                return;
            e.Handled = true;
            if (e.RightButton != MouseButtonState.Pressed)
                return;
            Point pos = e.GetPosition(MainList);
            if (pos.X < 50)
                return;
            HitTestResult result = VisualTreeHelper.HitTest(MainList, pos);
            if (result == null)
                return;
            ListViewItem selectedItem = Utils.FindVisualParent<ListViewItem>(result.VisualHit);
            if (selectedItem == null || selectedItem.DataContext == null)
                return;
            MonsterOrderInfo dataItem = selectedItem.DataContext as MonsterOrderInfo;
            if (string.IsNullOrEmpty(dataItem.MonsterName))
                return;
            DragDropAdorner adorner = new DragDropAdorner(selectedItem);
            mAdornerLayer = AdornerLayer.GetAdornerLayer(MainList);
            mAdornerLayer.Add(adorner);
            DataObject dataObject = new DataObject(dataItem);
            DragDrop.DoDragDrop(MainList, dataObject, DragDropEffects.Move);
            mAdornerLayer?.Remove(adorner);
            mAdornerLayer = null;
        }
        private void MainList_Drop(object sender, DragEventArgs e)
        {
            if (mIsLocked)
                return;
            Point pos = e.GetPosition(MainList);
            HitTestResult result = VisualTreeHelper.HitTest(MainList, pos);
            if (result == null)
                return;
            var sourcePerson = e.Data.GetData(typeof(MonsterOrderInfo)) as MonsterOrderInfo;
            if (sourcePerson == null)
                return;
            var listBoxItem = Utils.FindVisualParent<ListViewItem>(result.VisualHit);
            if (listBoxItem == null)
                return;
            var targetPerson = listBoxItem.Content as MonsterOrderInfo;
            if (ReferenceEquals(targetPerson, sourcePerson))
            {
                // check if finish order
                return;
            }
            int oldIndex = _orderCollection.IndexOf(sourcePerson);
            int newIndex = _orderCollection.IndexOf(targetPerson);
            if (oldIndex >= 0 && newIndex >= 0)
            {
                _orderCollection.Move(oldIndex, newIndex);
            }
        }

        public void AddBubble(AIBubbleInfo bubbleInfo)
        {
            if (bubbleInfo == null) return;

            Dispatcher.InvokeAsync(() =>
            {
                while (_bubbles.Count >= MAX_BUBBLES)
                {
                    RemoveBubble(_bubbles[0]);
                }

                var bubble = new AIBubbleControl();
                bubble.DataContext = bubbleInfo;
                bubble.Opacity = 0;

                _bubbles.Add(bubble);
                BubbleCanvas.Children.Add(bubble);

                UpdateBubblePositions();

                var storyboard = (Storyboard)FindResource("BubbleEnterStoryboard");
                storyboard = storyboard.Clone();
                storyboard.Begin(bubble);

                var timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(BUBBLE_INTERVAL_MS) };
                timer.Tick += (s, e) =>
                {
                    timer.Stop();
                    _bubbleTimers.Remove(timer);
                    PlayExitAnimation(bubble);
                };
                timer.Start();
                _bubbleTimers.Add(timer);
            });
        }

        private void RemoveBubble(AIBubbleControl bubble)
        {
            if (bubble == null || !_bubbles.Contains(bubble)) return;
            _bubbles.Remove(bubble);
            BubbleCanvas.Children.Remove(bubble);
        }

        private void UpdateBubblePositions()
        {
            double top = 0;
            for (int i = _bubbles.Count - 1; i >= 0; i--)
            {
                var bubble = _bubbles[i];
                bubble.UpdateLayout();
                double height = bubble.ActualHeight > 0 ? bubble.ActualHeight : 60;
                Canvas.SetTop(bubble, top);
                top += height + BUBBLE_MARGIN;
            }
        }

        private void PlayExitAnimation(AIBubbleControl bubble)
        {
            if (bubble == null || !_bubbles.Contains(bubble)) return;
            var storyboard = (Storyboard)FindResource("BubbleExitStoryboard");
            storyboard = storyboard.Clone();
            storyboard.Completed += (s, e) =>
            {
                RemoveBubble(bubble);
                UpdateBubblePositions();
            };
            storyboard.Begin(bubble);
        }

        private void CleanupBubbles()
        {
            foreach (var timer in _bubbleTimers)
            {
                timer.Stop();
            }
            _bubbleTimers.Clear();
            var enterStory = (Storyboard)FindResource("BubbleEnterStoryboard");
            var exitStory = (Storyboard)FindResource("BubbleExitStoryboard");
            foreach (var bubble in _bubbles)
            {
                if (enterStory != null) enterStory.Stop(bubble);
                if (exitStory != null) exitStory.Stop(bubble);
            }
            _bubbles.Clear();
            BubbleCanvas.Children.Clear();
        }
    }

    public class MonsterOrderInfo
    {
        public string AudienceName { set; get; }
        public string MonsterName { set; get; }
        public BitmapImage MonsterIcon { set; get; }
        public int GuardLevel { set; get; }

        // 历战等级：0 非历战，1 历战，2 历战王
        public int TemperedLevel { set; get; }

        public MonsterOrderInfo()
        {
        }
        public MonsterOrderInfo(string audience_name, string monster_name)
        {
            this.AudienceName = audience_name;
            this.MonsterName = monster_name;
        }

        public void Clear()
        {
            this.AudienceName = null;
            this.MonsterName = null;
        }
    }

    public class RollingInfo
    {
        public string Text { set; get; }
        public Color TextColor { set; get; }

        public RollingInfo(string text, Color color)
        {
            this.Text = text;
            this.TextColor = color;
        }
    }
}
