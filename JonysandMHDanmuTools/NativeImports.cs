using System;
using System.Runtime.InteropServices;

namespace MonsterOrderWindows
{
    internal static class NativeImports
    {
        private const string DllName = "MonsterOrderWilds.exe";

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern bool DataBridge_Initialize();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void DataBridge_Shutdown();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern bool DataBridge_IsMonsterDataLoaded();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern bool DataBridge_MatchMonsterName(
            string inputText,
            System.Text.StringBuilder outMonsterName,
            int nameBufferSize,
            out int outTemperedLevel);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void DataBridge_GetMonsterIconUrl(
            string monsterName,
            System.Text.StringBuilder outUrl,
            int urlBufferSize);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_SetValue(string key, string value, int type);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_Save();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_GetString(string key, System.Text.StringBuilder outValue, int bufferSize);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_GetBool(string key, out bool outValue);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_GetInt(string key, out int outValue);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_GetFloat(string key, out float outValue);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void Config_GetDouble(string key, out double outValue);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern bool PriorityQueue_Contains(string userId);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void PriorityQueue_Enqueue(
            string userId,
            long timeStamp,
            bool priority,
            string userName,
            string monsterName,
            int guardLevel,
            int temperedLevel);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void PriorityQueue_DequeueByIndex(int index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void PriorityQueue_SortQueue();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void PriorityQueue_Clear();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void PriorityQueue_SaveList();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern bool PriorityQueue_LoadList();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void PriorityQueue_GetAllNodes(OnQueueNodeCallback callback, IntPtr userData);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void DanmuProcessor_ProcessDanmu(string jsonStr);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void DataBridge_SetDanmuProcessedCallback(DanmuProcessedCallback callback, IntPtr userData);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void DataBridge_SetAIReplyCallback(OnAIReplyCallback callback, IntPtr userData);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void DataBridge_SetCheckinTTSPlayCallback(OnCheckinTTSPlayCallback callback, IntPtr userData);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void TTSManager_GetCurrentProviderName(System.Text.StringBuilder outBuffer, int bufferSize);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern bool ProfileManager_ExportCheckinRecords(string filePath);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate void DanmuProcessedCallback(string userName, string monsterName, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate void OnAIReplyCallback(string username, string content, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate void OnCheckinTTSPlayCallback(string username, string content, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate void OnQueueNodeCallback(string userId, long timeStamp, bool priority, string userName, string monsterName, int guardLevel, int temperedLevel, IntPtr userData);

        public enum ConfigFieldType
        {
            String = 0,
            Bool = 1,
            Int = 2,
            Float = 3,
            Double = 4
        }
    }
}
