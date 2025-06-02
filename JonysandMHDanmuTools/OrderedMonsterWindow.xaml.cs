using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
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
        // 点击计时
        private const uint ORDER_FINISH_CLICK_INTERVAL = 100; // in milliseconds
        // 拖曳时的画布
        private AdornerLayer mAdornerLayer = null;
        // 显示info的队列
        private Queue<RollingInfo> mInfoQueue;
        private DispatcherTimer mInfoChangeTimer;
        private const string _defaultInfo = "发送“点怪 xxx”进行点怪";
        // 界面是否锁定
        private bool mIsLocked = false;
        public OrderedMonsterWindow()
        {
            InitializeComponent();
            InfoText_Animation.Duration = new Duration(new TimeSpan(0, 0, 10));
            InfoText.Text = _defaultInfo;
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;

            GlobalEventListener.AddListener("AddRollingInfo", (object rollingInfo) => AddRollingInfo(rollingInfo as RollingInfo));
            GlobalEventListener.AddListener("RefreshOrder", (object _) => RefreshOrder());
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            this.Height = 360;
            this.Width = 440;

            // init timer
            mInfoChangeTimer = new DispatcherTimer();
            mInfoQueue = new Queue<RollingInfo>();
            mInfoChangeTimer.Interval = InfoText_Animation.Duration.TimeSpan;
            mInfoChangeTimer.Tick += new EventHandler(OnTimerTick);
            mInfoChangeTimer.Start();

            DanmuManager.GetInst().LoadHistoryOrder();

            // Hotkey.Regist(this, HotkeyModifiers.Alt, Key.Decimal, OnHotKeyLock);
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
            if (mIsLocked)
            {
                // 背景透明，且可以穿透
                MainWindow.Background = Brushes.Transparent;
                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                uint extendedStyle = GetWindowLong(hwnd, -20);
                SetWindowLong(hwnd, -20, extendedStyle | 0x20);
                Topmost = true;
                GlobalEventListener.Invoke("OrderWindowLocked", "");
                if (MainList.Items.Count > 0)
                    MainList.ScrollIntoView(MainList.Items[0]);
            }
            else
            {
                // 背景调黑，且可以操作窗口
                MainWindow.Background = Brushes.Gray;
                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                uint extendedStyle = GetWindowLong(hwnd, -20);
                SetWindowLong(hwnd, -20, extendedStyle ^ 0x20);
                Topmost = false;
            }
        }

        // 更新跑马灯消息
        private void OnTimerTick(object sender, EventArgs e)
        {
            if (mInfoQueue.Count == 0)
            {
                InfoText.Text = _defaultInfo;
                InfoText.Foreground = new SolidColorBrush(Colors.LightYellow);
            }
            else
            {
                var rollingInfo = mInfoQueue.Dequeue();
                InfoText.Text = rollingInfo.Text;
                InfoText.Foreground = new SolidColorBrush(rollingInfo.TextColor);
            }
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
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
        private void OnClickOrder(object sender, MouseButtonEventArgs e)
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
            PriorityQueue.GetInst().Dequeue(MainList.Items.IndexOf(orderInfo));
            RefreshOrder();
        }

        // 添加跑马灯消息
        public void AddRollingInfo(RollingInfo rollingInfo)
        {
            mInfoQueue.Enqueue(rollingInfo);
        }

        public void RefreshOrder()
        {
            // 应该不用加锁，每次队列变化时都会调用这里
            // 也就是说变化过后必定刷新一次
            Dispatcher.InvokeAsync(new Action(delegate
            {
                PriorityQueue.GetInst().SortQueue();
                MainList.Items.Clear();
                foreach (var items in PriorityQueue.GetInst().Queue)
                {
                    MonsterOrderInfo tempData = new MonsterOrderInfo();
                    tempData.AudienceName = items.UserName;
                    tempData.MonsterName = items.MonsterName;
                    tempData.GuardLevel = items.GuardLevel;
                    tempData.TemperedLevel = items.TemperedLevel;
                    string iconUrl = MonsterData.GetInst().GetMatchedMonsterIconUrl(tempData.MonsterName);
                    // icon 设置
                    if (!string.IsNullOrEmpty(iconUrl))
                        tempData.MonsterIcon = new Uri(iconUrl, UriKind.RelativeOrAbsolute);
                    MainList.Items.Add(tempData);
                }
            }));
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
            MainList.Items.Remove(sourcePerson);
            MainList.Items.Insert(MainList.Items.IndexOf(targetPerson), sourcePerson);
        }
    }

    public class MonsterOrderInfo
    {
        public string AudienceName { set; get; }
        public string MonsterName { set; get; }
        public Uri MonsterIcon { set; get; }
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
