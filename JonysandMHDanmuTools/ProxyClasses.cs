using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MonsterOrderWindows
{
    public class ConfigProxy : INotifyPropertyChanged
    {
        private static ConfigProxy _instance = null;
        public static ConfigProxy Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new ConfigProxy();
                return _instance;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private string _idCode = "";
        public string IdCode
        {
            get => _idCode;
            set { _idCode = value; OnPropertyChanged(); }
        }

        private bool _onlyMedalOrder = true;
        public bool OnlyMedalOrder
        {
            get => _onlyMedalOrder;
            set { _onlyMedalOrder = value; OnPropertyChanged(); }
        }

        private bool _enableVoice = false;
        public bool EnableVoice
        {
            get => _enableVoice;
            set { _enableVoice = value; OnPropertyChanged(); }
        }

        private int _speechRate = 0;
        public int SpeechRate
        {
            get => _speechRate;
            set { _speechRate = value; OnPropertyChanged(); }
        }

        private int _speechPitch = 0;
        public int SpeechPitch
        {
            get => _speechPitch;
            set { _speechPitch = value; OnPropertyChanged(); }
        }

        private int _speechVolume = 80;
        public int SpeechVolume
        {
            get => _speechVolume;
            set { _speechVolume = value; OnPropertyChanged(); }
        }

        private bool _onlySpeekWearingMedal = false;
        public bool OnlySpeekWearingMedal
        {
            get => _onlySpeekWearingMedal;
            set { _onlySpeekWearingMedal = value; OnPropertyChanged(); }
        }

        private int _onlySpeekGuardLevel = 0;
        public int OnlySpeekGuardLevel
        {
            get => _onlySpeekGuardLevel;
            set { _onlySpeekGuardLevel = value; OnPropertyChanged(); }
        }

        private bool _onlySpeekPaidGift = false;
        public bool OnlySpeekPaidGift
        {
            get => _onlySpeekPaidGift;
            set { _onlySpeekPaidGift = value; OnPropertyChanged(); }
        }

        private int _opacity = 80;
        public int Opacity
        {
            get => _opacity;
            set { _opacity = value; OnPropertyChanged(); }
        }

        private string _ttsEngine = "auto";
        public string TtsEngine
        {
            get => _ttsEngine;
            set { _ttsEngine = value; OnPropertyChanged(); }
        }

        private string _mimoApiKey = "";
        public string MimoApiKey
        {
            get => _mimoApiKey;
            set { _mimoApiKey = value; OnPropertyChanged(); }
        }

        private string _mimoVoice = "mimo_default";
        public string MimoVoice
        {
            get => _mimoVoice;
            set { _mimoVoice = value; OnPropertyChanged(); }
        }

        private string _mimoStyle = "";
        public string MimoStyle
        {
            get => _mimoStyle;
            set { _mimoStyle = value; OnPropertyChanged(); }
        }

        private float _mimoSpeed = 1.0f;
        public float MimoSpeed
        {
            get => _mimoSpeed;
            set { _mimoSpeed = value; OnPropertyChanged(); }
        }

        private double _topPosX = 0;
        public double TopPosX
        {
            get => _topPosX;
            set { _topPosX = value; OnPropertyChanged(); }
        }

        private double _topPosY = 0;
        public double TopPosY
        {
            get => _topPosY;
            set { _topPosY = value; OnPropertyChanged(); }
        }

        private string _defaultMarqueeText = "";
        public string DefaultMarqueeText
        {
            get => _defaultMarqueeText;
            set { _defaultMarqueeText = value; OnPropertyChanged(); }
        }

        private int _ttsCacheDaysToKeep = 7;
        public int TtsCacheDaysToKeep
        {
            get => _ttsCacheDaysToKeep;
            set { _ttsCacheDaysToKeep = value; OnPropertyChanged(); }
        }

        public void RefreshFromConfig(MainConfig config)
        {
            IdCode = config.ID_CODE ?? "";
            OnlyMedalOrder = config.ONLY_MEDAL_ORDER;
            EnableVoice = config.ENABLE_VOICE;
            SpeechRate = config.SPEECH_RATE;
            SpeechPitch = config.SPEECH_PITCH;
            SpeechVolume = config.SPEECH_VOLUME;
            OnlySpeekWearingMedal = config.ONLY_SPEEK_WEARING_MEDAL;
            OnlySpeekGuardLevel = config.ONLY_SPEEK_GUARD_LEVEL;
            OnlySpeekPaidGift = config.ONLY_SPEEK_PAID_GIFT;
            Opacity = config.OPACITY;
            TtsEngine = config.TTS_ENGINE ?? "auto";
            MimoApiKey = config.MIMO_API_KEY ?? "";
            MimoVoice = config.MIMO_VOICE ?? "mimo_default";
            MimoStyle = config.MIMO_STYLE ?? "";
            MimoSpeed = config.MIMO_SPEED;
            TopPosX = config.TopPos.X;
            TopPosY = config.TopPos.Y;
            DefaultMarqueeText = config.DEFAULT_MARQUEE_TEXT ?? "";
            TtsCacheDaysToKeep = config.TTS_CACHE_DAYS_TO_KEEP;
        }

        public void ApplyToConfig(MainConfig config)
        {
            config.ID_CODE = IdCode;
            config.ONLY_MEDAL_ORDER = OnlyMedalOrder;
            config.ENABLE_VOICE = EnableVoice;
            config.SPEECH_RATE = SpeechRate;
            config.SPEECH_PITCH = SpeechPitch;
            config.SPEECH_VOLUME = SpeechVolume;
            config.ONLY_SPEEK_WEARING_MEDAL = OnlySpeekWearingMedal;
            config.ONLY_SPEEK_GUARD_LEVEL = OnlySpeekGuardLevel;
            config.ONLY_SPEEK_PAID_GIFT = OnlySpeekPaidGift;
            config.OPACITY = Opacity;
            config.TTS_ENGINE = TtsEngine;
            config.MIMO_API_KEY = MimoApiKey;
            config.MIMO_VOICE = MimoVoice;
            config.MIMO_STYLE = MimoStyle;
            config.MIMO_SPEED = MimoSpeed;
            config.TopPos = new System.Windows.Point(TopPosX, TopPosY);
            config.DEFAULT_MARQUEE_TEXT = DefaultMarqueeText;
            config.TTS_CACHE_DAYS_TO_KEEP = TtsCacheDaysToKeep;
        }
    }

    public class QueueNodeProxy : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private string _userId = "";
        public string UserId
        {
            get => _userId;
            set { _userId = value; OnPropertyChanged(); }
        }

        private long _timeStamp = 0;
        public long TimeStamp
        {
            get => _timeStamp;
            set { _timeStamp = value; OnPropertyChanged(); }
        }

        private bool _priority = false;
        public bool Priority
        {
            get => _priority;
            set { _priority = value; OnPropertyChanged(); }
        }

        private string _userName = "";
        public string UserName
        {
            get => _userName;
            set { _userName = value; OnPropertyChanged(); }
        }

        private string _monsterName = "";
        public string MonsterName
        {
            get => _monsterName;
            set { _monsterName = value; OnPropertyChanged(); }
        }

        private int _guardLevel = 0;
        public int GuardLevel
        {
            get => _guardLevel;
            set { _guardLevel = value; OnPropertyChanged(); }
        }

        private int _temperedLevel = 0;
        public int TemperedLevel
        {
            get => _temperedLevel;
            set { _temperedLevel = value; OnPropertyChanged(); }
        }
    }

    public class PriorityQueueProxy : INotifyPropertyChanged
    {
        private static PriorityQueueProxy _instance = null;
        public static PriorityQueueProxy Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new PriorityQueueProxy();
                return _instance;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private System.Collections.Generic.List<QueueNodeProxy> _nodes = new System.Collections.Generic.List<QueueNodeProxy>();
        public System.Collections.Generic.List<QueueNodeProxy> Nodes
        {
            get => _nodes;
            set { _nodes = value; OnPropertyChanged(); }
        }

        private int _count = 0;
        public int Count
        {
            get => _count;
            set { _count = value; OnPropertyChanged(); }
        }

        public void RefreshFromQueue(PriorityQueue queue)
        {
            Nodes.Clear();
            foreach (var node in queue.Queue)
            {
                var proxy = new QueueNodeProxy
                {
                    UserId = node.UserId,
                    TimeStamp = node.TimeStamp,
                    Priority = node.Priority,
                    UserName = node.UserName,
                    MonsterName = node.MonsterName,
                    GuardLevel = node.GuardLevel,
                    TemperedLevel = node.TemperedLevel
                };
                Nodes.Add(proxy);
            }
            Count = queue.Count;
        }
    }

    public class MonsterDataProxy : INotifyPropertyChanged
    {
        private static MonsterDataProxy _instance = null;
        public static MonsterDataProxy Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new MonsterDataProxy();
                return _instance;
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private bool _isLoaded = false;
        public bool IsLoaded
        {
            get => _isLoaded;
            set { _isLoaded = value; OnPropertyChanged(); }
        }

        public void SetLoaded(bool loaded)
        {
            IsLoaded = loaded;
        }
    }

    public class EventDispatcher
    {
        private static EventDispatcher _instance = null;
        public static EventDispatcher Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new EventDispatcher();
                return _instance;
            }
        }

        public event Action OnConfigChanged;
        public event Action OnQueueChanged;
        public event Action<string, string> OnDanmuProcessed;

        public void NotifyConfigChanged()
        {
            OnConfigChanged?.Invoke();
        }

        public void NotifyQueueChanged()
        {
            OnQueueChanged?.Invoke();
        }

        public void NotifyDanmuProcessed(string userName, string monsterName)
        {
            OnDanmuProcessed?.Invoke(userName, monsterName);
        }
    }
}
