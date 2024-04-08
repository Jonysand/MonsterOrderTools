using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Windows.Threading;




namespace JonysandMHDanmuTools
{
    /// <summary>
    /// Interaction logic for OrderedMonsterWindow.xaml
    /// </summary>
    public partial class OrderedMonsterWindow : Window
    {
        // just for logging
        private ToolsMain mToolsMain = null;
        // 点击计时
        private const uint ORDER_FINISH_CLICK_INTERVAL = 100; // in milliseconds
        private DateTime mClickStartTime = DateTime.Now;
        private bool mClickStart = false;
        // 拖曳时的画布
        private AdornerLayer mAdornerLayer = null;
        // 显示info的队列
        private Queue<string> mInfoQueue;
        private DispatcherTimer mInfoChangeTimer;
        private const string _defaultInfo = "欢迎来到老白直播间，发送“点怪 xxx”进行点怪";
        public OrderedMonsterWindow(ToolsMain toolsMain)
        {
            mToolsMain = toolsMain;
            InitializeComponent();
            InfoText_Animation.Duration = new Duration(new TimeSpan(0, 0, 10));
            InfoText.Text = _defaultInfo;
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            this.Height = 360;
            this.Width = 400;

            // init timer
            mInfoChangeTimer = new DispatcherTimer();
            mInfoQueue = new Queue<string>();
            mInfoChangeTimer.Interval = InfoText_Animation.Duration.TimeSpan;
            mInfoChangeTimer.Tick += new EventHandler(OnTimerTick);
            mInfoChangeTimer.Start();

            // TEST
            // for (int i = 0; i < 16; i++)
            //    AddOrder("水友" + (i + 1).ToString(), "怪物名字最多八字" + (i+1).ToString());
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            e.Cancel = true;
            Hide();
        }

        // 更新跑马灯消息
        private void OnTimerTick(object sender, EventArgs e)
        {
            if (mInfoQueue.Count == 0)
                InfoText.Text = _defaultInfo;
            else
                InfoText.Text = mInfoQueue.Dequeue();
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
        }

        // 左键窗口任意位置拖曳
        [DllImport("user32.dll")]
        public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        private void OnClientAreaMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                IntPtr hwnd = new WindowInteropHelper(this).Handle;
                SendMessage(hwnd, 0x112, (IntPtr)0xF012, IntPtr.Zero);
            }
        }

        // 双击某个item以完成订单
        private void OnListItemDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                var pos = e.GetPosition(MainList);
                HitTestResult result = VisualTreeHelper.HitTest(MainList, pos);
                if (result == null)
                    return;
                var listViewItem = Utils.FindVisualParent<ListViewItem>(result.VisualHit);
                if (listViewItem == null)
                    return;
                PopOrder(MainList.Items.IndexOf(listViewItem.Content));
            }
        }

        // 添加点怪
        public void AddOrder(string audience_name, string monster_name)
        {
            MonsterOrderInfo newOrder = new MonsterOrderInfo();
            newOrder.AudienceName = audience_name;
            newOrder.MonsterName = monster_name;
            MainList.Items.Add(newOrder);
            // 标题提示
            AddRollingInfo(audience_name + " 点怪 " + monster_name + " 成功！");
        }

        // 完成（取消）点怪
        public void PopOrder(int index=0)
        {
            MainList.Items.RemoveAt(index);
            // 暂时处理成手动打完移除
            mToolsMain.RemoveRecord();
        }

        // 添加跑马灯消息
        public void AddRollingInfo(string msg)
        {
            mInfoQueue.Enqueue(msg);
        }

        // 拖拽排序 -------------------------------------
        private void MainList_QueryContinueDrag(object sender, QueryContinueDragEventArgs e)
        {
            if (mAdornerLayer == null)
                return;
            mAdornerLayer.Update();
        }
        private void MainList_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            mClickStartTime = DateTime.Now;
            mClickStart = true;
            e.Handled = true;
            if (e.LeftButton != MouseButtonState.Pressed)
                return;
            Point pos = e.GetPosition(MainList);
            HitTestResult result = VisualTreeHelper.HitTest(MainList, pos);
            if (result == null)
                return;
            ListViewItem selectedItem = Utils.FindVisualParent<ListViewItem>(result.VisualHit); // Find your actual visual you want to drag
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
                if (mClickStart)
                {
                    var timeinterval = (DateTime.Now - mClickStartTime).TotalMilliseconds;
                    mToolsMain.Log("[MainList_Drop] interval: " + timeinterval.ToString());
                    if (timeinterval < ORDER_FINISH_CLICK_INTERVAL)
                        PopOrder(MainList.Items.IndexOf(targetPerson));
                }
                mClickStart = false;
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

        public MonsterOrderInfo()
        {
        }
        public MonsterOrderInfo(string audience_name, string monster_name)
        {
            this.AudienceName = audience_name;
            this.MonsterName = monster_name;
        }
    }
}
