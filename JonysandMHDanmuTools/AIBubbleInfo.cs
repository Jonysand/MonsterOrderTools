using System;

namespace MonsterOrderWindows
{
    public class AIBubbleInfo
    {
        public string Username { get; set; }
        public string Content { get; set; }
        public DateTime Timestamp { get; set; }

        public AIBubbleInfo(string username, string content)
        {
            this.Username = username;
            this.Content = content;
            this.Timestamp = DateTime.Now;
        }
    }
}