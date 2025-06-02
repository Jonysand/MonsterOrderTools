using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Documents;
using Newtonsoft.Json;
using System.IO;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MonsterOrderWindows
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

    // 配置文件
    public class MainConfig
    {
        [JsonProperty]
        public Point TopPos { get => _topLeftPos; set { if (_topLeftPos != value) { _topLeftPos = value; OnPropertyChanged(); } } }

        // 窗口左上角坐标
        private Point _topLeftPos = new Point(0, 0);

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    public sealed class ConfigService
    {
        public MainConfig Config => _config;
        private readonly MainConfig _config;
        private readonly string _configDirectory;
        private readonly string _configFileName;
        private bool _configChanged = false;

        public ConfigService()
        {
            _configDirectory = Path.Combine(Environment.CurrentDirectory, @"MonsterOrderWilds_configs");
            _configFileName = "MainConfig.cfg";
            _config = new MainConfig();
            _config.PropertyChanged += OnConfigChanged;
        }

        public bool LoadConfig()
        {
            string configPath = Path.Combine(_configDirectory, _configFileName);
            if (File.Exists(configPath))
            {
                string json = File.ReadAllText(configPath);
                MainConfig config = JsonConvert.DeserializeObject<MainConfig>(json);
                _config.TopPos = config.TopPos;
                return true;
            }
            return false;
        }

        public void SaveConfig(bool force=false)
        {
            if (_config == null)
            {
                return;
            }
            if (!Directory.Exists(_configDirectory))
            {
                Directory.CreateDirectory(_configDirectory);
            }
            if (!force && !_configChanged)
                return;
            string configPath = Path.Combine(_configDirectory, _configFileName);
            File.WriteAllText(configPath, JsonConvert.SerializeObject(_config));
        }

        public void OnConfigChanged(object sender, PropertyChangedEventArgs e)
        {
            _configChanged = true;
        }
    }
}
