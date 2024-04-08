﻿using System;
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
        // 点击计时
        private const uint ORDER_FINISH_CLICK_INTERVAL = 100; // in milliseconds
        // 拖曳时的画布
        private AdornerLayer mAdornerLayer = null;
        // 显示info的队列
        private Queue<string> mInfoQueue;
        private DispatcherTimer mInfoChangeTimer;
        private const string _defaultInfo = "欢迎来到老白直播间，发送“点怪 xxx”进行点怪";
        public OrderedMonsterWindow()
        {
            InitializeComponent();
            InfoText_Animation.Duration = new Duration(new TimeSpan(0, 0, 10));
            InfoText.Text = _defaultInfo;
            InfoText_Animation.To = -InfoText.Text.Count() * InfoText.FontSize;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            this.Height = 360;
            this.Width = 440;

            // init timer
            mInfoChangeTimer = new DispatcherTimer();
            mInfoQueue = new Queue<string>();
            mInfoChangeTimer.Interval = InfoText_Animation.Duration.TimeSpan;
            mInfoChangeTimer.Tick += new EventHandler(OnTimerTick);
            mInfoChangeTimer.Start();
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

        private void OnClickOrder(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            MonsterOrderInfo orderInfo = button.DataContext as MonsterOrderInfo;
            PopOrder(MainList.Items.IndexOf(orderInfo));
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
            GlobalEventListener.Invoke("RemoveOrder", index);
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
            e.Handled = true;
            if (e.RightButton != MouseButtonState.Pressed)
                return;
            Point pos = e.GetPosition(MainList);
            if (pos.X < 50)
                return;
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
