using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows;
using System.Runtime.InteropServices;
using System.Windows.Documents;
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
        private static readonly object _lock = new object();

        public static void Invoke(string event_name, object args)
        {
            List<Action<object>> actions;
            lock (_lock)
            {
                if (!EventMap.TryGetValue(event_name, out actions))
                    return;
                actions = new List<Action<object>>(actions);
            }
            foreach (var action in actions)
            {
                action(args);
            }
        }

        public static void AddListener(string event_name, Action<object> action)
        {
            lock (_lock)
            {
                List<Action<object>> list;
                if (!EventMap.TryGetValue(event_name, out list))
                {
                    list = new List<Action<object>>();
                    EventMap[event_name] = list;
                }
                list.Add(action);
            }
        }

        public static void RemoveListener(string event_name, Action<object> action)
        {
            lock (_lock)
            {
                List<Action<object>> list;
                if (!EventMap.TryGetValue(event_name, out list))
                    return;
                list.Remove(action);
                if (list.Count == 0)
                {
                    EventMap.Remove(event_name);
                }
            }
        }
    };

    // 配置字段类型
    public enum ConfigFieldType
    {
        String = 0,
        Bool = 1,
        Int = 2,
        Float = 3,
        Double = 4
    }

    // 配置字段元数据（反射机制的核心）
    public class ConfigFieldInfo
    {
        public string Name { get; set; }           // C++ 中的字段名
        public ConfigFieldType Type { get; set; }  // 字段类型
        public Func<object> Getter { get; set; }  // C# 侧 getter
        public Action<object> Setter { get; set; } // C# 侧 setter（接受 object 以便统一调用）

        public object Get() => Getter();
        public void Set(object value) => Setter(value);
    }

    // 配置字段注册表（简单反射机制）
    public static class ConfigFieldRegistry
    {
        private static readonly Dictionary<string, ConfigFieldInfo> _fields = new Dictionary<string, ConfigFieldInfo>();

        static ConfigFieldRegistry()
        {
            Register("idCode", ConfigFieldType.String,
                () => GetString("idCode"),
                v => SetValue("idCode", (string)v, ConfigFieldType.String));

            Register("onlyMedalOrder", ConfigFieldType.Bool,
                () => GetBool("onlyMedalOrder"),
                v => SetValue("onlyMedalOrder", (bool)v, ConfigFieldType.Bool));

            Register("enableVoice", ConfigFieldType.Bool,
                () => GetBool("enableVoice"),
                v => SetValue("enableVoice", (bool)v, ConfigFieldType.Bool));

            Register("speechRate", ConfigFieldType.Int,
                () => GetInt("speechRate"),
                v => SetValue("speechRate", (int)v, ConfigFieldType.Int));

            Register("speechPitch", ConfigFieldType.Int,
                () => GetInt("speechPitch"),
                v => SetValue("speechPitch", (int)v, ConfigFieldType.Int));

            Register("speechVolume", ConfigFieldType.Int,
                () => GetInt("speechVolume"),
                v => SetValue("speechVolume", (int)v, ConfigFieldType.Int));

            Register("onlySpeekWearingMedal", ConfigFieldType.Bool,
                () => GetBool("onlySpeekWearingMedal"),
                v => SetValue("onlySpeekWearingMedal", (bool)v, ConfigFieldType.Bool));

            Register("onlySpeekGuardLevel", ConfigFieldType.Int,
                () => GetInt("onlySpeekGuardLevel"),
                v => SetValue("onlySpeekGuardLevel", (int)v, ConfigFieldType.Int));

            Register("onlySpeekPaidGift", ConfigFieldType.Bool,
                () => GetBool("onlySpeekPaidGift"),
                v => SetValue("onlySpeekPaidGift", (bool)v, ConfigFieldType.Bool));

            Register("opacity", ConfigFieldType.Int,
                () => GetInt("opacity"),
                v => SetValue("opacity", (int)v, ConfigFieldType.Int));

            Register("penetratingModeOpacity", ConfigFieldType.Int,
                () => GetInt("penetratingModeOpacity"),
                v => SetValue("penetratingModeOpacity", (int)v, ConfigFieldType.Int));

            Register("ttsEngine", ConfigFieldType.String,
                () => GetString("ttsEngine"),
                v => SetValue("ttsEngine", (string)v, ConfigFieldType.String));

            Register("mimoApiKey", ConfigFieldType.String,
                () => GetString("mimoApiKey"),
                v => SetValue("mimoApiKey", (string)v, ConfigFieldType.String));

            Register("mimoVoice", ConfigFieldType.String,
                () => GetString("mimoVoice"),
                v => SetValue("mimoVoice", (string)v, ConfigFieldType.String));

            Register("mimoStyle", ConfigFieldType.String,
                () => GetString("mimoStyle"),
                v => SetValue("mimoStyle", (string)v, ConfigFieldType.String));

            Register("mimoSpeed", ConfigFieldType.Float,
                () => GetFloat("mimoSpeed"),
                v => SetValue("mimoSpeed", (float)v, ConfigFieldType.Float));

            Register("minimaxVoiceId", ConfigFieldType.String,
                () => GetString("minimaxVoiceId"),
                v => SetValue("minimaxVoiceId", (string)v, ConfigFieldType.String));

            Register("minimaxSpeed", ConfigFieldType.Float,
                () => GetFloat("minimaxSpeed"),
                v => SetValue("minimaxSpeed", (float)v, ConfigFieldType.Float));

            Register("topPosX", ConfigFieldType.Double,
                () => GetDouble("topPosX"),
                v => SetValue("topPosX", (double)v, ConfigFieldType.Double));

            Register("topPosY", ConfigFieldType.Double,
                () => GetDouble("topPosY"),
                v => SetValue("topPosY", (double)v, ConfigFieldType.Double));

            Register("defaultMarqueeText", ConfigFieldType.String,
                () => GetString("defaultMarqueeText"),
                v => SetValue("defaultMarqueeText", (string)v, ConfigFieldType.String));

            Register("ttsCacheDaysToKeep", ConfigFieldType.Int,
                () => GetInt("ttsCacheDaysToKeep"),
                v => SetValue("ttsCacheDaysToKeep", (int)v, ConfigFieldType.Int));

            Register("enableCaptainCheckinAI", ConfigFieldType.Bool,
                () => GetBool("enableCaptainCheckinAI"),
                v => SetValue("enableCaptainCheckinAI", (bool)v, ConfigFieldType.Bool));

            Register("checkinTriggerWords", ConfigFieldType.String,
                () => GetString("checkinTriggerWords"),
                v => SetValue("checkinTriggerWords", (string)v, ConfigFieldType.String));
        }

        public static void Register(string name, ConfigFieldType type, Func<object> getter, Action<object> setter)
        {
            _fields[name] = new ConfigFieldInfo { Name = name, Type = type, Getter = getter, Setter = setter };
        }

        public static object Get(string name)
        {
            if (_fields.TryGetValue(name, out var field))
                return field.Get();
            throw new ArgumentException($"Config field '{name}' not found");
        }

        public static void Set(string name, object value)
        {
            if (_fields.TryGetValue(name, out var field))
                field.Set(value);
            else
                throw new ArgumentException($"Config field '{name}' not found");
        }

        public static bool Contains(string name) => _fields.ContainsKey(name);

        public static IEnumerable<string> AllFields => _fields.Keys;

        // 内部调用 NativeImports
        private static string GetString(string key)
        {
            var sb = new System.Text.StringBuilder(256);
            NativeImports.Config_GetString(key, sb, 256);
            return sb.ToString();
        }

        private static bool GetBool(string key)
        {
            NativeImports.Config_GetBool(key, out bool value);
            return value;
        }

        private static int GetInt(string key)
        {
            NativeImports.Config_GetInt(key, out int value);
            return value;
        }

        private static float GetFloat(string key)
        {
            NativeImports.Config_GetFloat(key, out float value);
            return value;
        }

        private static double GetDouble(string key)
        {
            NativeImports.Config_GetDouble(key, out double value);
            return value;
        }

        private static void SetValue(string key, object value, ConfigFieldType type)
        {
            string stringValue = value?.ToString() ?? "";
            NativeImports.Config_SetValue(key, stringValue, (int)type);
        }
    }

    // 配置文件（使用反射机制）
    public class MainConfig
    {
        // 使用 ConfigFieldRegistry 进行反射式访问
        public Point TopPos
        {
            get => new Point((double)ConfigFieldRegistry.Get("topPosX"), (double)ConfigFieldRegistry.Get("topPosY"));
            set
            {
                ConfigFieldRegistry.Set("topPosX", value.X);
                ConfigFieldRegistry.Set("topPosY", value.Y);
                OnPropertyChanged();
            }
        }

        public String ID_CODE
        {
            get => (string)ConfigFieldRegistry.Get("idCode");
            set { ConfigFieldRegistry.Set("idCode", value); OnPropertyChanged(); }
        }

        public bool ONLY_MEDAL_ORDER
        {
            get => (bool)ConfigFieldRegistry.Get("onlyMedalOrder");
            set { ConfigFieldRegistry.Set("onlyMedalOrder", value); OnPropertyChanged(); }
        }

        public bool ENABLE_VOICE
        {
            get => (bool)ConfigFieldRegistry.Get("enableVoice");
            set { ConfigFieldRegistry.Set("enableVoice", value); OnPropertyChanged(); }
        }

        public int SPEECH_RATE
        {
            get => (int)ConfigFieldRegistry.Get("speechRate");
            set { ConfigFieldRegistry.Set("speechRate", value); OnPropertyChanged(); }
        }

        public int SPEECH_PITCH
        {
            get => (int)ConfigFieldRegistry.Get("speechPitch");
            set { ConfigFieldRegistry.Set("speechPitch", value); OnPropertyChanged(); }
        }

        public int SPEECH_VOLUME
        {
            get => (int)ConfigFieldRegistry.Get("speechVolume");
            set { ConfigFieldRegistry.Set("speechVolume", value); OnPropertyChanged(); }
        }

        public bool ONLY_SPEEK_WEARING_MEDAL
        {
            get => (bool)ConfigFieldRegistry.Get("onlySpeekWearingMedal");
            set { ConfigFieldRegistry.Set("onlySpeekWearingMedal", value); OnPropertyChanged(); }
        }

        public int ONLY_SPEEK_GUARD_LEVEL
        {
            get => (int)ConfigFieldRegistry.Get("onlySpeekGuardLevel");
            set { ConfigFieldRegistry.Set("onlySpeekGuardLevel", value); OnPropertyChanged(); }
        }

        public bool ONLY_SPEEK_PAID_GIFT
        {
            get => (bool)ConfigFieldRegistry.Get("onlySpeekPaidGift");
            set { ConfigFieldRegistry.Set("onlySpeekPaidGift", value); OnPropertyChanged(); }
        }

        public int OPACITY
        {
            get => (int)ConfigFieldRegistry.Get("opacity");
            set { ConfigFieldRegistry.Set("opacity", value); OnPropertyChanged(); }
        }

        public int PENETRATING_MODE_OPACITY
        {
            get => (int)ConfigFieldRegistry.Get("penetratingModeOpacity");
            set { ConfigFieldRegistry.Set("penetratingModeOpacity", value); OnPropertyChanged(); }
        }

        public String TTS_ENGINE
        {
            get => (string)ConfigFieldRegistry.Get("ttsEngine");
            set { ConfigFieldRegistry.Set("ttsEngine", value); OnPropertyChanged(); }
        }

        public String MIMO_API_KEY
        {
            get => (string)ConfigFieldRegistry.Get("mimoApiKey");
            set { ConfigFieldRegistry.Set("mimoApiKey", value); OnPropertyChanged(); }
        }

        public String MIMO_VOICE
        {
            get => (string)ConfigFieldRegistry.Get("mimoVoice");
            set { ConfigFieldRegistry.Set("mimoVoice", value); OnPropertyChanged(); }
        }

        public String MIMO_STYLE
        {
            get => (string)ConfigFieldRegistry.Get("mimoStyle");
            set { ConfigFieldRegistry.Set("mimoStyle", value); OnPropertyChanged(); }
        }

        public float MIMO_SPEED
        {
            get => (float)ConfigFieldRegistry.Get("mimoSpeed");
            set { ConfigFieldRegistry.Set("mimoSpeed", value); OnPropertyChanged(); }
        }

        public String MINIMAX_VOICE_ID
        {
            get => (string)ConfigFieldRegistry.Get("minimaxVoiceId");
            set { ConfigFieldRegistry.Set("minimaxVoiceId", value); OnPropertyChanged(); }
        }

        public float MINIMAX_SPEED
        {
            get => (float)ConfigFieldRegistry.Get("minimaxSpeed");
            set { ConfigFieldRegistry.Set("minimaxSpeed", value); OnPropertyChanged(); }
        }

        public String DEFAULT_MARQUEE_TEXT
        {
            get => (string)ConfigFieldRegistry.Get("defaultMarqueeText");
            set { ConfigFieldRegistry.Set("defaultMarqueeText", value); OnPropertyChanged(); }
        }

        public int TTS_CACHE_DAYS_TO_KEEP
        {
            get => (int)ConfigFieldRegistry.Get("ttsCacheDaysToKeep");
            set { ConfigFieldRegistry.Set("ttsCacheDaysToKeep", value); OnPropertyChanged(); }
        }

        public bool ENABLE_CAPTAIN_CHECKIN_AI
        {
            get => (bool)ConfigFieldRegistry.Get("enableCaptainCheckinAI");
            set { ConfigFieldRegistry.Set("enableCaptainCheckinAI", value); OnPropertyChanged(); }
        }

        public string CHECKIN_TRIGGER_WORDS
        {
            get => (string)ConfigFieldRegistry.Get("checkinTriggerWords");
            set { ConfigFieldRegistry.Set("checkinTriggerWords", value); OnPropertyChanged(); }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    public sealed class ConfigService
    {
        public MainConfig Config => _config;
        private MainConfig _config;

        public void LoadConfig()
        {
            _config = new MainConfig();
        }

        public void SaveConfig()
        {
            NativeImports.Config_Save();
        }

        public MainConfig GetConfig()
        {
            return _config;
        }
    }
}
