using System;
using System.Windows.Media;

namespace MonsterOrderWindows
{
    internal class DanmuManager
    {
        private static NativeImports.DanmuProcessedCallback danmuProcessedCallback;
        private static NativeImports.OnAIReplyCallback aiReplyCallback;
        private static NativeImports.OnCheckinTTSPlayCallback checkinTTSPlayCallback;

        static DanmuManager _Inst = null;

        public static DanmuManager GetInst()
        {
            if (_Inst == null)
                _Inst = new DanmuManager();
            return _Inst;
        }

        public DanmuManager()
        {
            danmuProcessedCallback = OnDanmuProcessed;
            NativeImports.DataBridge_SetDanmuProcessedCallback(danmuProcessedCallback, IntPtr.Zero);

            if (!ToolsMain.IsOnlyOrderMonster)
            {
                aiReplyCallback = OnAIReplyCallback;
                NativeImports.DataBridge_SetAIReplyCallback(aiReplyCallback, IntPtr.Zero);

                checkinTTSPlayCallback = OnCheckinTTSPlayCallback;
                NativeImports.DataBridge_SetCheckinTTSPlayCallback(checkinTTSPlayCallback, IntPtr.Zero);
            }
        }

        public void LoadHistoryOrder()
        {
            PriorityQueue.GetInst().LoadList();
            GlobalEventListener.Invoke("RefreshOrder", null);
        }

        public void ClearHistoryOrder()
        {
            PriorityQueue.GetInst().Clear();
            GlobalEventListener.Invoke("RefreshOrder", null);
        }

        private static void OnDanmuProcessed(string userName, string monsterName, IntPtr userData)
        {
            GlobalEventListener.Invoke("RefreshOrder", null);
            GlobalEventListener.Invoke("AddRollingInfo", new RollingInfo(userName + " 点怪 " + monsterName + " 成功！", Colors.Yellow));
        }

        private static void OnAIReplyCallback(string username, string content, IntPtr userData)
        {
            var bubbleInfo = new AIBubbleInfo(username, content);
            GlobalEventListener.Invoke("AIReplyBubble", bubbleInfo);
        }

        private static void OnCheckinTTSPlayCallback(string username, string content, IntPtr userData)
        {
            var bubbleInfo = new AIBubbleInfo(username, content);
            GlobalEventListener.Invoke("AIReplyBubble", bubbleInfo);
        }
    }
}
