using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Windows.Media.Imaging;

namespace MonsterOrderWindows
{
    public static class MonsterIconLoader
    {
        private static string _zipPath;
        private static readonly Dictionary<string, BitmapImage> _cache = new Dictionary<string, BitmapImage>();
        private static readonly object _lock = new object();

        public static void Initialize(string configDirectory)
        {
            _zipPath = Path.Combine(configDirectory, "monster_icons.zip");
            ToolsMain.SendCommand("Log:MonsterIconLoader initialized with zip=" + _zipPath);
        }

        public static BitmapImage LoadIcon(string zipEntryPath)
        {
            if (string.IsNullOrEmpty(zipEntryPath))
                return null;

            lock (_lock)
            {
                if (_cache.TryGetValue(zipEntryPath, out var cached))
                    return cached;
            }

            if (!File.Exists(_zipPath))
            {
                ToolsMain.SendCommand("Log:MonsterIconLoader zip not found=" + _zipPath);
                return null;
            }

            try
            {
                using (var archive = ZipFile.OpenRead(_zipPath))
                {
                    var entry = archive.GetEntry(zipEntryPath);
                    if (entry == null)
                    {
                        ToolsMain.SendCommand("Log:MonsterIconLoader entry not found=" + zipEntryPath);
                        return null;
                    }

                    using (var stream = entry.Open())
                    {
                        // ZipEntry 的流不可 seek，需要先复制到 MemoryStream
                        var memoryStream = new MemoryStream();
                        stream.CopyTo(memoryStream);
                        memoryStream.Position = 0;

                        var bitmap = new BitmapImage();
                        bitmap.BeginInit();
                        bitmap.StreamSource = memoryStream;
                        bitmap.CacheOption = BitmapCacheOption.OnLoad;
                        bitmap.EndInit();
                        bitmap.Freeze();

                        lock (_lock)
                        {
                            _cache[zipEntryPath] = bitmap;
                        }
                        ToolsMain.SendCommand("Log:MonsterIconLoader loaded=" + zipEntryPath);
                        return bitmap;
                    }
                }
            }
            catch (Exception ex)
            {
                ToolsMain.SendCommand("Log:MonsterIconLoader error loading " + zipEntryPath + "=" + ex.Message);
                return null;
            }
        }

        public static void ClearCache()
        {
            lock (_lock)
            {
                _cache.Clear();
            }
        }
    }
}
