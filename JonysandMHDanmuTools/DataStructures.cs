using System;

namespace MonsterOrderWindows
{
    /// <summary>
    /// 配置数据批量结构（值类型，用于C++/C#跨层批量传输）
    /// 对应C++层的ConfigData结构
    /// </summary>
    public struct ConfigDataSnapshot
    {
        // 基本配置
        public string IdCode;
        public bool OnlyMedalOrder;
        public bool EnableVoice;
        public int SpeechRate;
        public int SpeechPitch;
        public int SpeechVolume;
        public bool OnlySpeekWearingMedal;
        public int OnlySpeekGuardLevel;
        public bool OnlySpeekPaidGift;
        public int Opacity;

        // MiMo TTS 配置
        public string TtsEngine;
        public string MimoApiKey;
        public string MimoVoice;
        public string MimoStyle;
        public string MimoDialect;
        public string MimoRole;
        public string MimoAudioFormat;
        public float MimoSpeed;

        // 窗口位置
        public double TopPosX;
        public double TopPosY;

        /// <summary>
        /// 从现有MainConfig创建快照
        /// </summary>
        public static ConfigDataSnapshot FromMainConfig(MainConfig config)
        {
            return new ConfigDataSnapshot
            {
                IdCode = config.ID_CODE ?? "",
                OnlyMedalOrder = config.ONLY_MEDAL_ORDER,
                EnableVoice = config.ENABLE_VOICE,
                SpeechRate = config.SPEECH_RATE,
                SpeechPitch = config.SPEECH_PITCH,
                SpeechVolume = config.SPEECH_VOLUME,
                OnlySpeekWearingMedal = config.ONLY_SPEEK_WEARING_MEDAL,
                OnlySpeekGuardLevel = config.ONLY_SPEEK_GUARD_LEVEL,
                OnlySpeekPaidGift = config.ONLY_SPEEK_PAID_GIFT,
                Opacity = config.OPACITY,
                TtsEngine = config.TTS_ENGINE ?? "auto",
                MimoApiKey = config.MIMO_API_KEY ?? "",
                MimoVoice = config.MIMO_VOICE ?? "mimo_default",
                MimoStyle = config.MIMO_STYLE ?? "",
                MimoDialect = "",
                MimoRole = "",
                MimoAudioFormat = "mp3",
                MimoSpeed = config.MIMO_SPEED,
                TopPosX = config.TopPos.X,
                TopPosY = config.TopPos.Y
            };
        }

        /// <summary>
        /// 应用快照到MainConfig
        /// </summary>
        public void ApplyTo(MainConfig config)
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
        }
    }

    /// <summary>
    /// 队列节点数据批量结构（值类型）
    /// 对应C++层的QueueNodeData结构
    /// </summary>
    public struct QueueNodeSnapshot
    {
        public string UserId;
        public long TimeStamp;
        public bool Priority;
        public string UserName;
        public string MonsterName;
        public int GuardLevel;
        public int TemperedLevel;
    }

    /// <summary>
    /// 怪物数据批量结构（值类型）
    /// </summary>
    public struct MonsterInfoSnapshot
    {
        public string MonsterName;
        public string IconUrl;
        public int TemperedLevel;
    }
}
