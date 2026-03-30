using System;
using System.Windows.Media;

namespace MonsterOrderWindows
{
    internal class DanmuManager
    {
        private static NativeImports.DanmuProcessedCallback danmuProcessedCallback;

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
    }
}
