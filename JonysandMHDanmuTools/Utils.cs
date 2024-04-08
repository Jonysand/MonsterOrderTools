using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Documents;
using System.Diagnostics.Tracing;
using System.Windows.Input;
using System.Windows.Interop;

namespace JonysandMHDanmuTools
{
    internal class Utils
    {
        public static T FindVisualParent<T>(DependencyObject obj) where T : class
        {
            while (obj != null)
            {
                if (obj is T)
                    return obj as T;
                obj = VisualTreeHelper.GetParent(obj);
            }
            return null;
        }
    }

    public class DragDropAdorner : Adorner
    {
        public DragDropAdorner(UIElement parent)
            : base(parent)
        {
            IsHitTestVisible = false;
            mDraggedElement = parent as FrameworkElement;
        }
        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);
            if (mDraggedElement != null)
            {
                Win32.POINT screenPos = new Win32.POINT();
                if (Win32.GetCursorPos(ref screenPos))
                {
                    Point pos = PointFromScreen(new Point(screenPos.X, screenPos.Y));
                    // Point elementPos2 = mDraggedElement.PointToScreen(new Point());
                    Rect rect = new Rect(pos.X - mDraggedElement.ActualWidth / 2, pos.Y - mDraggedElement.ActualHeight / 2, mDraggedElement.ActualWidth, mDraggedElement.ActualHeight);
                    drawingContext.PushOpacity(1.0);
                    Brush highlight = mDraggedElement.TryFindResource(SystemColors.HighlightBrushKey) as Brush;
                    if (highlight != null)
                        drawingContext.DrawRectangle(highlight, new Pen(Brushes.Red, 0), rect);
                    drawingContext.DrawRectangle(new VisualBrush(mDraggedElement),
                        new Pen(Brushes.Transparent, 0), rect);
                    drawingContext.Pop();
                }
            }
        }
        FrameworkElement mDraggedElement = null;
    }

    public static class Win32
    {
        public struct POINT { public Int32 X; public Int32 Y; }
        [DllImport("user32.dll")]
        public static extern bool GetCursorPos(ref POINT point);
    }

    // Event Listener
    public class GlobalEventListener
    {
        private static Dictionary<string, List<Action<object>>> EventMap= new Dictionary<string, List<Action<object>>>();
        public static void Invoke(string event_name, object args)
        {
            foreach (var action in EventMap[event_name])
            {
                action(args);
            }
        }

        public static void AddListener(string event_name, Action<object> action)
        {
            if (!EventMap.ContainsKey(event_name))
            {
                EventMap[event_name] = new List<Action<object>>();
            }
            EventMap[event_name].Add(action);
        }

        public static void RemoveListener(string event_name, Action<object> action)
        {
            if (!EventMap.ContainsKey(event_name))
            {
                return;
            }
            EventMap[event_name].Remove(action);
            if (EventMap[event_name].Count() == 0)
            {
                EventMap.Remove(event_name);
            }
        }
    };

    // HotKeys
    public static class Hotkey
    {
        #region API
        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool RegisterHotKey(IntPtr hWnd, int id, HotkeyModifiers fsModifiers, uint vk);

        [DllImport("user32.dll")]
        static extern bool UnregisterHotKey(IntPtr hWnd, int id);
        #endregion

        /// <summary>
        /// 注册快捷键
        /// </summary>
        /// <param name="window">持有快捷键的窗口</param>
        /// <param name="fsModifiers">组合键</param>
        /// <param name="key">快捷键</param>
        /// <param name="callBack">回调函数</param>
        public static void Regist(Window window, HotkeyModifiers fsModifiers, Key key, HotKeyCallBackHanlder callBack)
        {
            var hwnd = new WindowInteropHelper(window).Handle;
            var _hwndSource = HwndSource.FromHwnd(hwnd);
            _hwndSource.AddHook(WndProc);

            int id = keyid++;
            var vk = KeyInterop.VirtualKeyFromKey(key);

            if (!RegisterHotKey(hwnd, id, fsModifiers, (uint)vk))
                throw new Exception("注册快捷键失败。");

            keymap[id] = callBack;
        }

        /// <summary>
        /// 快捷键消息处理
        /// </summary>
        static IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            if (msg == WM_HOTKEY)
            {
                int id = wParam.ToInt32();
                if (keymap.TryGetValue(id, out var callback))
                {
                    callback();
                }
            }
            return IntPtr.Zero;
        }

        /// <summary>
        /// 注销快捷键
        /// </summary>
        /// <param name="hWnd">持有快捷键的窗口句柄</param>
        /// <param name="callBack">回调函数</param>
        public static void UnRegist(IntPtr hWnd, HotKeyCallBackHanlder callBack)
        {
            foreach (KeyValuePair<int, HotKeyCallBackHanlder> var in keymap)
            {
                if (var.Value == callBack)
                    UnregisterHotKey(hWnd, var.Key);
            }
        }

        const int WM_HOTKEY = 0x312;
        static int keyid = 10;
        static Dictionary<int, HotKeyCallBackHanlder> keymap = new Dictionary<int, HotKeyCallBackHanlder>();

        public delegate void HotKeyCallBackHanlder();
    }

    public enum HotkeyModifiers
    {
        Alt = 0x1,
        Ctrl = 0x2,
        Shift = 0x4,
        Win = 0x8
    }
}
